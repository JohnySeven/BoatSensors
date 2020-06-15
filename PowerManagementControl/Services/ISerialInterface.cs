namespace PowerManagementControl.Services
{
    public interface ISerialInterface
    {
        bool RunQuery(string query, out string[] response);
    }
}