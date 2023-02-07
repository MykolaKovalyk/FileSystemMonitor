using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;


class DataManager
{
    public static T Load<T>(string path)
    {
         return JsonConvert.DeserializeObject<T>(
             File.ReadAllText(path));
    }


    public static void Save<T>(T data, string path)
    {
        string parentDir = Path.GetDirectoryName(path);
        if(parentDir?.Length > 0)
            Directory.CreateDirectory(parentDir);

        File.WriteAllText(path, JsonConvert.SerializeObject(data, Formatting.Indented));
    }
}
