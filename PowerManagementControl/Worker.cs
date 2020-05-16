using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;

namespace PowerManagementControl
{
    public class Worker : BackgroundService
    {
        private readonly ILogger<Worker> _logger;
        private readonly SerialInterface _serial;

        public Worker(ILogger<Worker> logger, SerialInterface serial)
        {
            _logger = logger;
            _serial = serial;
        }

        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                if(_serial.RunQuery("uptime", out string[] response))
                {
                    _logger.LogInformation("Uptime: " + response[1]);
                }
                
                await Task.Delay(1000, stoppingToken);
            }

            _logger.LogInformation("Worker is shuting down...");
        }
    }
}
