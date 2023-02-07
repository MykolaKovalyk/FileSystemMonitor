using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading.Channels;
using System.Threading.Tasks;
using FileSystemMonitor;

class DataParser
{
    
    static ChangeRecord record;

    static DateTime timeOfLastSave;
    static DateTime timeOfLastLogOutput;
    static int changesRecordedSinceLastSave = 0;

    static volatile bool isAlive = true;

    public static async Task StartHandling(Channel<(uint pid, string process, string file)> channel)
    {
        isAlive = true;

        if (Program.config.saveAfterChangesRecorded <= 0 &&
            Program.config.saveChangesEverySeconds <= 0)
            throw new InvalidDataException("At least one of saveAfterChangesRecorded and saveChangesEverySeconds should be bigger than 0, but not both of them!");

        if (Program.config.outputLogFileEverySeconds <= 0 &&
            Program.config.maxLogFileSize <= 0)
            throw new InvalidDataException("At least one of outputLogFileEverySeconds and maxLogFileSize should be bigger than 0, but not both of them!");

        try
        {
             record = DataManager.Load<ChangeRecord>(Program.config.activeLogPath);
        }
        catch(IOException)
        {
            CreateNewRecord();
        }

        timeOfLastSave = DateTime.Now;
        timeOfLastLogOutput = record.endTime;

        try
        {
            while (isAlive)
            {
                (uint pid, string process, string file) = await channel.Reader.ReadAsync();

                
                RecordChanges(process, file);


                changesRecordedSinceLastSave++;


                if (IsItLongEnoughToOutput() || IsLogFileBigEnoughToOputput())
                {
                    Console.WriteLine("Outputting the log...");

                    OutputSavedLogFile();
                    CreateNewRecord();

                    timeOfLastLogOutput = DateTime.Now;
                    
                    timeOfLastSave = DateTime.Now;
                    changesRecordedSinceLastSave = 0;
                }
                else if (IsItLongEnoughToSave() || WereThereEnoughChangesToSave())
                {
                    Console.WriteLine("Saving changes...");

                    SaveChanges();

                    timeOfLastSave = DateTime.Now;
                    changesRecordedSinceLastSave = 0;
                }


            }
        }
        catch(ChannelClosedException)
        {
            return;
        }
    }

    public static void FinishHandling() => isAlive = false;
        


    static void RecordChanges(string process, string file)
    {
        string[] pathFolderStructure = Utilities.SplitPath(file);


        Directory processChangeList;

        if (record.changes.ContainsKey(process))
            processChangeList = record.changes[process];
        else
        {
            processChangeList = new Directory();
            record.changes[process] = processChangeList;
        }



        Directory current = processChangeList;
        foreach (string dir in pathFolderStructure)
        {
            if(!current.ContainsKey(dir))   
                current[dir] = new Directory();

            current = current[dir];
        }

        record.endTime = DateTime.Now;
    }


    static void SaveChanges()
    {
        DataManager.Save(record, Program.config.activeLogPath);
    }

    static void OutputSavedLogFile()
    {
        DataManager.Save(record, Path.Combine(Program.config.outputDir, $"record_{record.startTime.ToString("yyyy_MM_dd__HH_mm_ss")}.json"));
    }

    static void CreateNewRecord()
    {
        record = new ChangeRecord();
        
        record.startTime = DateTime.Now;
        record.endTime = DateTime.Now;

        DataManager.Save(record, Program.config.activeLogPath);
    }


    static bool IsItLongEnoughToSave()
    {
        return (DateTime.Now - timeOfLastSave).TotalSeconds > Program.config.saveChangesEverySeconds 
            && Program.config.saveChangesEverySeconds > 0;
    }

    static bool WereThereEnoughChangesToSave()
    {
        return changesRecordedSinceLastSave > Program.config.saveAfterChangesRecorded
            && Program.config.saveAfterChangesRecorded > 0;
    }

    static bool IsItLongEnoughToOutput()
    {
        return (DateTime.Now - timeOfLastLogOutput).TotalSeconds > Program.config.outputLogFileEverySeconds
            && Program.config.outputLogFileEverySeconds > 0;
    }

    static bool IsLogFileBigEnoughToOputput()
    {
        return File.Exists(Program.config.activeLogPath) &&  
            new FileInfo(Program.config.activeLogPath).Length > Program.config.maxLogFileSize 
            && Program.config.maxLogFileSize > 0;
    }


    public class Directory : Dictionary<string, Directory> { }

    public class ChangeRecord
    {
        public DateTime startTime;
        public DateTime endTime;

        public Dictionary<string, Directory> changes = new();
    }
}
