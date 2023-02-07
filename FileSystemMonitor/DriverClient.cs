using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Threading.Channels;


class DriverClient
{
    private static StringBuilder processBuilder = new StringBuilder(512);
    private static StringBuilder fileBuilder = new StringBuilder(512);


    [DllImport(@"FSMClient.dll")]
    public static extern int Connect();

    [DllImport(@"FSMClient.dll")]
    public static extern int Disonnect();

    [MethodImpl(MethodImplOptions.AggressiveOptimization | MethodImplOptions.AggressiveInlining)]
    public static int GetMessage(out uint pid, out string process, out string file)
    {
        pid = 0;

        int processPathSize = 0;
        int filePathSize = 0;



        int result = ReceiveMessage(ref pid, processBuilder, fileBuilder, ref processPathSize, ref filePathSize);



        if (result < 0)      // receive failed
        {
            process = null;
            file = null;
        }
        else
        {
            process = processBuilder.ToString(0, processPathSize);
            file = fileBuilder.ToString(0, filePathSize);
        }



        return result;
    }

    [MethodImpl(MethodImplOptions.AggressiveOptimization | MethodImplOptions.AggressiveInlining)]
    public static void StartReceiving(Channel<(uint pid, string process, string file)> channel)
    {
        while (GetMessage(out uint pid, out string process, out string file) >= 0)
            channel.Writer.TryWrite((pid, process, file));

        channel.Writer.Complete();
    }


    [DllImport(@"FSMClient.dll", CharSet = CharSet.Unicode)]
    private static extern int ReceiveMessage(ref uint pid, [MarshalAs(UnmanagedType.LPWStr)] StringBuilder processPath, [MarshalAs(UnmanagedType.LPWStr)] StringBuilder filePath, ref int processPathLength, ref int filePathLength);





}
