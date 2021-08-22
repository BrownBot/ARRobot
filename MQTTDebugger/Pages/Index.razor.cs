using System;
using System.Net.Http;
using System.Net.Http.Json;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Components;
using MQTTnet;
using MQTTnet.Client;
using MQTTnet.Client.Options;
using MqttShared.Models;

namespace MQTTDebugger.Pages
{
    public partial class Index : ComponentBase
    {
        [Inject]
        HttpClient client { get; set; }

        public Index()
        {
            Legs[0] = (byte)'X';
        }

        public byte[] Legs { get; set; } = new byte[13];

        private async Task OnSentClick()
        {
            var payload = new LegsPayload()
            {
                Legs = this.Legs
            };

            
            var result = await client.PostAsJsonAsync("robot", payload);
            Console.WriteLine(result.StatusCode);
        }

    }


}