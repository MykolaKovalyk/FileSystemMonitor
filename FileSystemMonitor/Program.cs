using System;
using System.Text;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using System.Threading.Channels;


namespace FileSystemMonitor
{
    public class Configurations
    {
        public string outputDir;
        public string activeLogPath;

        public float saveChangesEverySeconds;
        public int saveAfterChangesRecorded;

        public float outputLogFileEverySeconds;
        public int maxLogFileSize;

        public int maxConnectAttempts;

        public HashSet<string> ignoreProcesses;

    }


    public class Program
    {
       public static Configurations config;

        static async Task Main(string[] args)
        {
            int status = 0;



            try
            {
                config = DataManager.Load<Configurations>("config.json");
            }
            catch
            {
                DataManager.Save(new Configurations(), "config.json");
                Console.WriteLine("No config-file was found. A basic one was created instead.");
                return;
            }



            for(int i = 0; i < config.maxConnectAttempts; i++)
            {
                status = DriverClient.Connect();

                if (status < 0)
                {
                    Console.WriteLine($"Connection attempt failed with the code {status}, retrying...");
                    Thread.Sleep(750);
                }
                else
                    break;
            }

            if (status < 0)
            {
                Console.WriteLine($"Establishing connection with the driver failed with the code {status}");
                return;
            }
            else
                Console.WriteLine($"Successfully established connection with the driver.");




            Channel<(uint, string, string)> interThreadChannel = Channel.CreateUnbounded<(uint, string, string)>(); ;


            Console.WriteLine($"Starting message-receiving loop...");
            Task receiveTask = Task.Factory.StartNew(() =>

                DriverClient.StartReceiving(interThreadChannel),
                TaskCreationOptions.LongRunning);

            Console.WriteLine($"Starting message-handling loop...");
            Task handleTask = DataParser.StartHandling(interThreadChannel);


            Console.WriteLine($"Ready.");


            await receiveTask;

            Console.WriteLine($"Message-receiveing loop terminated, terminating the handling-loop too...");
            DataParser.FinishHandling();
            await handleTask;
        }
    }
}
