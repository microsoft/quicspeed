using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace quicspeed.UWP
{
    public sealed partial class MainPage
    {
        [DllImport("quicspeednative", CallingConvention = CallingConvention.Cdecl)]
        public static extern void QSInitialize();

        public MainPage()
        {
            this.InitializeComponent();

            QSInitialize();

            LoadApplication(new quicspeed.App());
        }
    }
}
