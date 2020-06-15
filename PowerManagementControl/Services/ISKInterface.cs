using System;
using PowerManagementControl.Models;

namespace PowerManagementControl.Services
{
    public interface ISKInterface : IDisposable
    {
        bool IsConnected {get; }
        void SendUpdate<T>(string path, T value);
        ObservableValue<T> GetValue<T>(string path);

        PutRequest GetPutRequest();
    }
}