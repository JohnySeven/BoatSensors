using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using PowerManagementControl.Models;
using PowerManagementControl.Services;

namespace PowerManagementControl
{
    public class Worker : BackgroundService
    {
        private readonly ILogger<Worker> _logger;
        private readonly ISerialInterface _serial;
        private readonly ISKInterface _signalK;

        public Worker(ILogger<Worker> logger, ISerialInterface serial, ISKInterface signalK)
        {
            _logger = logger;
            _serial = serial;
            _signalK = signalK;
        }

        private void ParsePins(string[] pins)
        {
            foreach (var pin in pins)
            {
                if (ParseLine(pin, out Dictionary<string, string> values))
                {
                    _logger.LogTrace($"PIN UPDATE: {values["pin"]} = {values["state"]}");
                    _signalK.SendUpdate($"switches.{values["pin"]}.state", values["state"] == "1");
                }
                else
                {
                    _logger.LogWarning("PARSE ERROR: " + pin);
                }
            }
        }

        private void ParseAdc(string[] pins)
        {
            foreach (var line in pins)
            {
                if (ParseLine(line, out Dictionary<string, string> values))
                {
                    if (values.ContainsKey("adc"))
                    {
                        _signalK.SendUpdate($"sensors.adc{values["adc"]}.voltage", double.Parse(values["value"], CultureInfo.InvariantCulture));
                    }
                    else
                    {
                        _logger.LogError($"Wrong ADC format: {line}!");
                    }
                }
            }
        }

        private bool ParseLine(string line, out Dictionary<string, string> args)
        {
            //pin=0,state=1
            line = line.Trim();

            try
            {
                if (line.Contains(","))
                {
                    args = line.Split(",")
                                .Select(p => p.Split('=', 2))
                                .Select(p => new { key = p[0], value = p[1] })
                                .ToDictionary(k => k.key, v => v.value);

                    return true;
                }
                else
                {
                    args = null;
                    return false;
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, line);
                args = null;
                return false;
            }
        }

        private void InvokeSafe(string name, Action action)
        {
            try
            {
                action();
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, $"Action {name} failure!");
            }
        }

        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            var it = 0;
            try
            {
                while (!stoppingToken.IsCancellationRequested)
                {
                    it++;
                    //_logger.LogDebug($"Worker loop start it={it}.");

                    if (it == 1)
                    {
                        InvokeSafe("Query pin status", () =>
                        {
                            if (_serial.RunQuery("pins", out string[] pinList))
                            {
                                //_logger.LogInformation(string.Join(";", pinList));
                                ParsePins(pinList);
                            }
                        });
                    }
                    else if (it == 2)
                    {
                        InvokeSafe("Query ADC", () =>
                        {
                            if (_serial.RunQuery("adc", out string[] adcList))
                            {
                                ParseAdc(adcList);
                            }
                        });
                    }
                    else if (it == 3)
                    {
                        InvokeSafe("Query events", () =>
                        {
                            if (_serial.RunQuery("events", out string[] events))
                            {
                                _logger.LogInformation(string.Join(";", events));
                            }
                        });

                        InvokeSafe("Query uptime", () =>
                        {
                            if (_serial.RunQuery("uptime", out string[] uptime))
                            {
                                if (long.TryParse(uptime.Last(), out long uptimeNumber))
                                {
                                    _signalK.SendUpdate("pmu.uptime", uptimeNumber);
                                }
                            }
                        });
                    }
                    else if (it > 4)
                    {
                        it = 0;
                    }

                    var request = _signalK.GetPutRequest();

                    while (request != null)
                    {
                        foreach (var path in request.Puts)
                        {
                            _logger.LogDebug($"Got {path.Path} put request with value {path.Value}.");

                            if (path.Path.StartsWith("switches."))
                            {
                                if (int.TryParse(path.Path.Split('.')[1], out int pin))
                                {
                                    var state = (bool)path.Value == true ? "h" : "l";
                                    if (pin != 0) //not rpi power pin
                                    {
                                        if (_serial.RunQuery($"power -pin {pin} -state \"{state}\" -delay 0", out string[] response))
                                        {
                                            _signalK.SendUpdate($"switches.{pin}.state", (bool)path.Value);
                                        }
                                    }
                                }
                            }

                        }

                        request = _signalK.GetPutRequest();
                    }

                    await Task.Delay(250, stoppingToken);
                }

                _logger.LogInformation("Worker is shuting down...");
            }
            catch (Exception ex)
            {
                _logger.LogCritical(ex, "Worker failure!");
            }

            if (!File.Exists("noshutdown"))
            {
                _serial.RunQuery("power -pin 0 -state \"h\" -delay 0", out string[] dummy);

                if (_serial.RunQuery("power -pin 0 -state \"l\" -delay 45000", out string[] response))
                {
                    _logger.LogInformation("Full power down has been scheduled!");

                }
            }
            else
            {
                _logger.LogWarning("Noshutdown file found, will not schedule power down.");
            }
        }
    }
}
