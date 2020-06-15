using System;
using Newtonsoft.Json;

namespace PowerManagementControl.Models
{
    public class SKUpdateQueueItem
    {
        public string Path {get; set;}
        public object Value {get; set;}
    }
}