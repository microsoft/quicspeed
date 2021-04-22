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

        public MainPage()
        {
            InitializeComponent();

            QSInitialize();
        }
    }
}
