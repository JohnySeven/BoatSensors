#if DEBUG
//#define FAKE_SERIAL_INTERFACE
#endif
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using PowerManagementControl.Services;
using PowerManagementControl.Services.Dummy;

namespace PowerManagementControl
{
    public class Program
    {
        public static void Main(string[] args)
        {
            CreateHostBuilder(args).Build().Run();
        }

        public static IHostBuilder CreateHostBuilder(string[] args) =>
            Host.CreateDefaultBuilder(args)
                .ConfigureServices((hostContext, services) =>
                {
                    #if FAKE_SERIAL_INTERFACE
                    services.AddSingleton(typeof(ISerialInterface), typeof(DummyInterface));
                    #else
                    services.AddSingleton(typeof(ISerialInterface), typeof(SerialInterface));
                    #endif
                    services.AddSingleton(typeof(ISKInterface), typeof(SKInterface));
                    services.AddHostedService<Worker>();
                    services.AddLogging();
                });
    }
}
