using System;
using System.ComponentModel;
using System.Linq;
using Newtonsoft.Json.Linq;

namespace PowerManagementControl.Models
{
    public abstract class ObservableValue : INotifyPropertyChanged
    {
        public DateTime? LastUpdate { get; protected set; }
        public string Path { get; protected set; }

        public string Source { get; protected set; }
        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string name, object value)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        internal abstract void UpdateValue(object value, DateTime? updateDate);

        internal abstract bool ValueEquals(object value);

        public static ObservableValue FromValue(string path, string source, JToken value, DateTime? lastUpdate)
        {
            ObservableValue ret;

            if (value is JValue simpleValue)
            {
                var type = typeof(ObservableValue<>).MakeGenericType(simpleValue.Value.GetType());

                ret = (ObservableValue)type.GetConstructors().First().Invoke(new[] { path, source });
            }
            else if (value is JObject)
            {
                ret = new ObservableValue<JObject>(path, source);
            }
            else
            {
                throw new NotSupportedException(value.ToString());
            }

            ret.UpdateValue(value, lastUpdate);

            return ret;
        }
    }

    public class ObservableValue<T> : ObservableValue
    {

        public ObservableValue(string path, string source)
        {
            Path = path;
            Source = source;
        }

        private T _value = default(T);

        public T Value
        {
            get => _value;
            internal set
            {
                _value = value;
                OnPropertyChanged(nameof(Value), value);

                System.Diagnostics.Debug.WriteLine($"{Source}.{Path}={Value}");
            }
        }

        internal override bool ValueEquals(object value)
        {
            return Object.Equals(_value, value);
        }

        internal override void UpdateValue(object value, DateTime? updateDate)
        {
            if (value is JValue jsonValue)
            {
                Value = (T)Convert.ChangeType(jsonValue.Value, typeof(T));
            }
            else
            {
                Value = (T)value;
            }
            LastUpdate = updateDate ?? DateTime.UtcNow;
        }
    }
}