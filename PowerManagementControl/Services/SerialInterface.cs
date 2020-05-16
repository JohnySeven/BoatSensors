using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Threading;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;

public class SerialInterface
{
    SerialPort _port;
    readonly ILogger<SerialInterface> _logger;

    public SerialInterface(IConfiguration configuration, ILogger<SerialInterface> logger)
    {
        _logger = logger;
        var serialSection = configuration.GetSection("Serial");
        _logger.LogInformation("Opening port...");
        _port = new SerialPort(serialSection.GetValue("port", ""), serialSection.GetValue("BaudRate", 115200));

        _port.Open();
        _logger.LogInformation("Port is open");
    }

    public bool RunQuery(string query, out string[] response)
    {
        _port.WriteLine(query);

        var lines = new List<string>();

        Thread.Sleep(100);

        while(_port.BytesToRead > 0)
        {
            var line = _port.ReadLine();

            //_logger.LogDebug("LINE: " + line);

            if(line.StartsWith("OK"))
            {
                response = lines.ToArray();
                return true;
            }
            else if(line.StartsWith("ERROR"))
            {
                response = new [] { line };

                return false;
            }
            else
            {
                lines.Add(line);
            }
        }
        response = null;
        return false;
    }
}