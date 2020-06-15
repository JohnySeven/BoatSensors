using System;
using System.Collections.Generic;
using Newtonsoft.Json;

namespace PowerManagementControl.Models
{
    public class PutRequest
    {
        //{"requestId":"84125f32-c7c7-4707-a898-af0985687e83","put":[{"path":"sensors.gpio0.state","value":false}]}

        [JsonProperty("requestId")]
        public Guid RequestId {get; set;}

        [JsonProperty("put")]
        public IList<PathValuePair> Puts {get; set;}
    }


}