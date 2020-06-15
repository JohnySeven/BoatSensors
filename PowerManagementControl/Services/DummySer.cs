using System.Linq;
using Microsoft.Extensions.Logging;

namespace PowerManagementControl.Services.Dummy
{
    public class DummyInterface : ISerialInterface
    {

        public DummyInterface(ILogger<DummyInterface> logger)
        {
            logger.LogWarning($"Dummy interface has been initialized!");
        }
        private int iteration = 0;
        private bool[] Pins = new [] { true, false, false, false };
        public bool RunQuery(string query, out string[] response)
        {
            iteration++;

            if(query == "uptime")
            {
                response = new string[] { "# uptime", iteration.ToString()};
                return true;
            }
            else if(query == "pins")
            {
                var pin = 0;

                var states = Pins.Select(p => new 
                {
                    Pin = pin++,
                    State = p
                });

                response = new [] { "# pins" }.Union(states.Select(s => $"pin={s.Pin},state={(s.State ? 1 : 0)}"))
                                 .ToArray();
/*<<< # pins
dbug: PowerManagementControl.Services.SerialInterface[0]
\\49  <<< pin=0,state=1
dbug: PowerManagementControl.Services.SerialInterface[0]
\\34  <<< pin=1,state=0
dbug: PowerManagementControl.Services.SerialInterface[0]
\\19  <<< pin=2,state=0
dbug: PowerManagementControl.Services.SerialInterface[0]
\\4   <<< pin=3,state=0
dbug: PowerManagementControl.Services.SerialInterface[0]
\\0   <<< OK*/
                //response = new string[] { $"pin=0,state={(Pins[0] ? "1" : "0")}", $"pin=1,state={(Pins[1] ? "1" : "0")}", $"pin=2,state={(Pins[2] ? "1" : "0")}", $"pin=3,state={(Pins[3] ? "1" : "0")}"};
                return true;
            }
            else if(query == "adc")
            {
                response = new string[] { "# adc", "adc=0,value=14.77", "adc=1,value=2.0"};
                return true;
            }
            else if(query == "events")
            {
                if(iteration == 1)
                {
                    response = new string[] { "# events", "event=1,argument=1,time=0" };
                }
                else
                {
                    response = new string[] { "" };
                }
                return true;
            }
            else if(query.StartsWith("power"))
            {
                var pin = int.Parse(query.Substring("power -pin ".Length, 1));
                Pins[pin] = query.Contains("\"h\"");
                response = new string[] { "# power", "OK" };
                return true;
            }
            else
            {
                response = new string[] { "ERROR: not supported"};
                return false;
            }
        }
    }
}