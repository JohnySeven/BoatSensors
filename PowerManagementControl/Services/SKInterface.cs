using System;
using System.IO;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using PowerManagementControl.Models;
using Newtonsoft.Json;
using System.Net.Http;
using System.Text;
using System.Net;
using Newtonsoft.Json.Linq;
using System.Threading.Tasks;
using System.Threading;
using System.Collections;
using System.Linq;
using System.Collections.Concurrent;
using System.Collections.Generic;
using WebSocketSharp;

namespace PowerManagementControl.Services
{
    public class SKInterface : ISKInterface
    {
        private const string TokenFilePath = "token.json";
        WebSocket _socket = null;
        TokenModel _token = null;
        readonly ILogger<SKInterface> _logger = null;
        readonly string _server = null;
        readonly int _serverPort = 80;
        Thread _workerThread;
        CancellationTokenSource _cancellationToken;

        readonly Queue<SKUpdateQueueItem> _updatesQueue = new Queue<SKUpdateQueueItem>();
        readonly Queue<PutRequest> _putRequests = new Queue<PutRequest>();

        public PutRequest GetPutRequest() => _putRequests.Count > 0 ? _putRequests.Dequeue() : null;

        readonly ConcurrentDictionary<string, ObservableValue> _paths = new ConcurrentDictionary<string, ObservableValue>();
        public SKInterface(IConfiguration config, ILogger<SKInterface> logger)
        {
            _logger = logger;
            var section = config.GetSection("SignalK");
            _server = section.GetValue<string>("Address");
            _serverPort = section.GetValue<int>("Port");

            StartWork();

            _cancellationToken = new CancellationTokenSource();
        }

        private void StartWork()
        {
            _workerThread = new Thread(WorkerRun);
            _workerThread.Name = "SignalK worker";
            _workerThread.IsBackground = true;
            _workerThread.Start();
        }

        private async void WorkerRun(object obj)
        {

            var token = _cancellationToken.Token;

            while (!token.WaitHandle.WaitOne(250))
            {
                if (!IsConnected)
                {
                    if (await InitializeConnection().ConfigureAwait(true))
                    {
                        IsConnected = true;
                    }
                }

                if (_socket.IsAlive)
                {
                    try
                    {
                        if (_updatesQueue.Count > 0)
                        {
                            var updates = new UpdatesMessageModel()
                            {
                                Updates = new List<UpdateSource>(new [] 
                                {
                                    new UpdateSource() 
                                    {
                                        Source = "PMC",
                                        Values = new List<PathValuePair>(),
                                        TimeStamp = DateTime.Now
                                    }
                                }),
                                Context = "vessels.self"
                            };

                            //var cmd = @"{""updates"": [{""$source"": ""SOURCE-HERE"",""values"":[".Replace("SOURCE-HERE", _instance);
                            for (int i = 0; i < 10 && _updatesQueue.Count > 0; i++)
                            {
                                var item = _updatesQueue.Dequeue();
                                updates.Updates.First().Values.Add(new PathValuePair()
                                {
                                    Path = item.Path,
                                    Value = JToken.FromObject(item.Value)
                                });
                            }

                            var updatesJson = JsonConvert.SerializeObject(updates, Formatting.None);
                            _socket.Send(updatesJson);
                        }
                    }
                    catch (WebSocketException wex)
                    {

                    }
                }
                else
                {
                    IsConnected = false;
                }
            }
        }

        private void LoadToken()
        {
            if (File.Exists(TokenFilePath))
            {
                _token = JsonConvert.DeserializeObject<TokenModel>(File.ReadAllText(TokenFilePath));
            }
            else
            {
                _logger.LogInformation($"Token file {TokenFilePath} doesn't exist!");

                _token = new TokenModel()
                {
                    ClientId = Guid.NewGuid()
                };

                SaveToken();
            }
        }

        private void SaveToken()
        {
            if (_token != null)
            {
                var json = JsonConvert.SerializeObject(_token);

                File.WriteAllText(TokenFilePath, json);
            }
        }

        private async Task<bool> InitializeConnection()
        {
            var ret = false;

            try
            {
                LoadToken();

                var tokenIsOK = false;

                if (string.IsNullOrEmpty(_token.Token))
                {
                    tokenIsOK = await LoadTokenFromServer();
                }
                else
                {
                    tokenIsOK = true;
                }

                if (tokenIsOK && !string.IsNullOrEmpty(_token.Token))
                {
                    _socket = new WebSocket("ws://" + _server + ":" + _serverPort + "/signalk/v1/stream?subscribe=none&token=" + _token.Token);
                    _socket.OnMessage += (s, m) =>
                    {
                        _logger.LogDebug(m.Data);
                        if(m.IsText)
                        {
                            var json = m.Data;
                            var message = JObject.Parse(json);

                            //{"requestId":"84125f32-c7c7-4707-a898-af0985687e83","put":[{"path":"sensors.gpio0.state","value":false}]}

                            if (message["name"] != null)
                            {
                                _logger.LogDebug("Welcome message received!");

                                _logger.LogDebug(json);
                            }
                            else if(message.ContainsKey("put"))
                            {
                                //put request
                                var putRequest = JsonConvert.DeserializeObject<PutRequest>(json);
                                _putRequests.Enqueue(putRequest);
                            }
                            else if (message["context"] != null)
                            {
                                var updates = JsonConvert.DeserializeObject<UpdatesMessageModel>(json);

                                foreach (var update in updates.Updates)
                                {
                                    foreach (var path in update.Values)
                                    {
                                        if (_paths.TryGetValue(path.Path, out ObservableValue value))
                                        {
                                            value.UpdateValue(path.Value, update.TimeStamp);
                                        }
                                        else
                                        {
                                            _paths.TryAdd(path.Path, ObservableValue.FromValue(path.Path, update.Source, path.Value, update.TimeStamp));
                                        }
                                    }
                                }
                                /*{"context":"vessels.urn:mrn:signalk:uuid:44f7d9c3-5fd4-468b-96aa-2fc86b89cbdc","updates":[{"source":{"sentence":"RMC","talker":"GP","type":"NMEA0183","label":"GPS"},"$source":"GPS.GP","timestamp":"2014-04-03T08:57:59.000Z","values":[{"path":"navigation.magneticVariation","value":0}]}]}*/
                            }
                            else
                            {
                                _logger.LogDebug(json);
                            }
                        }
                    };
                    _socket.Connect();
                    ret = true;
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Failed to initialize connection!");
            }

            return ret;
        }

        private async Task<bool> LoadTokenFromServer()
        {
            var ret = false;

            var http = new HttpClient();

            http.BaseAddress = new Uri("http://" + _server + ":" + _serverPort + "/", UriKind.Absolute);

            var requestObject = new
            {
                clientId = _token.ClientId.ToString(),
                description = "Power Management"
            };

            while (!ret)
            {
                var content = new StringContent(JsonConvert.SerializeObject(requestObject), Encoding.UTF8, "application/json");
                var response = await http.PostAsync(new Uri("signalk/v1/access/requests", UriKind.Relative), content);

                _logger.LogInformation($"Token request result code={response.StatusCode}");

                if (response.StatusCode == HttpStatusCode.Accepted)
                {
                    var responseJson = await response.Content.ReadAsStringAsync();

                    _logger.LogDebug("Got response " + responseJson);

                    var serverResponse = JObject.Parse(responseJson);

                    if (serverResponse["state"].ToString() == "PENDING")
                    {
                        var tokenPoolHref = new Uri(serverResponse["href"].ToString(), UriKind.Relative);

                        while (!ret)
                        {
                            _logger.LogInformation($"Checking token request status from uri {tokenPoolHref}...");
                            response = await http.GetAsync(tokenPoolHref);
                            responseJson = await response.Content.ReadAsStringAsync();
                            _logger.LogDebug("Got response " + responseJson);
                            serverResponse = JObject.Parse(responseJson);

                            var requestState = serverResponse["state"].ToString();

                            if (requestState == "COMPLETED")
                            {
                                var accessRequest = serverResponse["accessRequest"];
                                var permission = accessRequest["permission"].ToString();

                                if (permission == "APPROVED")
                                {
                                    _token.Token = accessRequest["token"].ToString();
                                    _logger.LogInformation("Access has been approved!");
                                    SaveToken();
                                    ret = true;
                                    continue;
                                }
                                else
                                {
                                    _logger.LogInformation("Token access DENIED!");
                                    return false;
                                }
                            }

                            Thread.Sleep(3000);
                        }
                    }
                    else
                    {
                        ret = false;
                    }
                }
            }

            return ret;
        }

        public bool IsConnected { get; private set; }

        public void Dispose()
        {
            _cancellationToken.Cancel();
        }

        public ObservableValue<T> GetValue<T>(string path)
        {
            return (ObservableValue<T>)_paths.GetOrAdd(path, p => new ObservableValue<T>(path, null));
        }

        public void SendUpdate<T>(string path, T value)
        {
            if(value is float)
            {
                throw new ArgumentException("Please use double, not float!!");
            }

            if(_paths.TryGetValue(path, out ObservableValue skValue))
            {
                if(skValue.ValueEquals(value))
                {
                    return;
                }
            }

            _logger.LogTrace($"SK: {path} = {value}");

            _updatesQueue.Enqueue(new SKUpdateQueueItem()
            {
                Path = path,
                Value = value
            });
        }
    }
}