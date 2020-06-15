using System;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json.Serialization;

namespace PowerManagementControl.Models
{
    public class UpdatesMessageModel
    {
        [JsonProperty("context")]
        public string Context {get; set;}

        [JsonProperty("updates")]
        public IList<UpdateSource> Updates {get; set;}
    }

    public class UpdateSource
    {
        [JsonProperty("$source")]
        public string Source {get; set;}

        [JsonProperty("timestamp")]
        public DateTime? TimeStamp {get; set;}

        [JsonProperty("values")]
        public IList<PathValuePair> Values {get; set;}
    }

    public class PathValuePair
    {
        [JsonProperty("path")]
        public string Path {get; set;}

        [JsonProperty("value")]
        public JToken Value {get; set;}
    }
/*
{
    "context": "vessels.urn:mrn:signalk:uuid:44f7d9c3-5fd4-468b-96aa-2fc86b89cbdc",
    "updates": [
        {
            "source": {
                "sentence": "RMC",
                "talker": "GP",
                "type": "NMEA0183",
                "label": "GPS"
            },
            "$source": "GPS.GP",
            "timestamp": "2014-04-03T08:57:59.000Z",
            "values": [
                {
                    "path": "navigation.magneticVariation",
                    "value": 0
                }
            ]
        }
    ]
}

*/
}