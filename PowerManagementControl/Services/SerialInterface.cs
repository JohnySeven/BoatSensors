using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Threading;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;

namespace PowerManagementControl.Services
{
    public class SerialInterface : ISerialInterface
    {
        SerialPort _port;
        readonly ILogger<SerialInterface> _logger;
        private readonly bool _logCommunication;

        public SerialInterface(IConfiguration configuration, ILogger<SerialInterface> logger)
        {
            _logger = logger;
            var serialSection = configuration.GetSection("Serial");
            _logger.LogInformation("Opening port...");
            _port = new SerialPort(serialSection.GetValue("port", ""), serialSection.GetValue("BaudRate", 115200));
            _port.DtrEnable = false;

            _logCommunication = serialSection.GetValue("Log", false);
            _port.ReadTimeout = 1000;
            _port.Open();
            _logger.LogInformation("Port is open");
        }

        public bool RunQuery(string query, out string[] response)
        {
            var ret = false;
            try
            {
                if (_logCommunication)
                {
                    _logger.LogDebug($">>> " + query);
                }

                _port.DiscardInBuffer();
                _port.DiscardOutBuffer();
                _port.WriteLine(query);
                Thread.Sleep(250);

                var lines = new List<string>();
                var timeout = DateTime.UtcNow.AddSeconds(5);

                while (timeout > DateTime.UtcNow)
                {
                    var line = _port.ReadLine();
                    _logger.LogDebug($"<<< " + line);

                    //_logger.LogDebug("LINE: " + line);

                    if (line.StartsWith("OK") || line.StartsWith("Done"))
                    {
                        response = lines.ToArray();
                        return true;
                    }
                    else if (line.StartsWith("ERROR"))
                    {
                        response = new[] { line };
                        return false;
                    }
                    else
                    {
                        lines.Add(line);
                    }
                }
            }
            catch (TimeoutException)
            {
                _logger.LogWarning("Read timeout!");
                //close and open port
                _port.Close();
                _port.Open();
            }

            _logger.LogError("Power module communication timeout!");

            response = null;
            return ret;
        }
    }
}