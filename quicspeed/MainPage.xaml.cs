using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Xamarin.Forms;

namespace quicspeed
{
    public partial class MainPage : ContentPage
    {
        [DllImport("quicspeednative", CallingConvention = CallingConvention.Cdecl)]
        public static extern void QSInitialize();

        [DllImport("quicspeednative", CallingConvention = CallingConvention.Cdecl)]
        public static extern void QSRunTransfer(IntPtr ResultHandler);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void QS_RESULT_DELEGATE(ulong SpeedKbps);

        QS_RESULT_DELEGATE MyHandler;

        public MainPage()
        {
            InitializeComponent();

            MyHandler = TransferResultHandler;

            QSInitialize();
        }

        void Handle_Clicked(object sender, System.EventArgs e)
        {
            RunTransfer();
        }

        void RunTransfer(QS_RESULT_DELEGATE ResultHandler) // TODO - Keep Delegate Alive!!
        {
            var ResultHandlerPtr = Marshal.GetFunctionPointerForDelegate(ResultHandler);
            QSRunTransfer(ResultHandlerPtr);
        }

        void RunTransfer()
        {
            RunTransfer(MyHandler);
        }

        void TransferResultHandler(ulong SpeedKbps)
        {
            Device.BeginInvokeOnMainThread(() => { MyLabel.Text = string.Format("{0}", SpeedKbps); });
        }
    }
}
