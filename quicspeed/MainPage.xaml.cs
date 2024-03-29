﻿/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

using System;
using System.Runtime.InteropServices;
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

        QS_RESULT_DELEGATE ResultDelegate;

        public MainPage()
        {
            InitializeComponent();
            ResultDelegate = TransferResultHandler;
            QSInitialize();
        }

        void DownloadClicked(object sender, System.EventArgs e)
        {
            DownloadButton.IsEnabled = false;
            QSRunTransfer(Marshal.GetFunctionPointerForDelegate(ResultDelegate));
        }

        void TransferResultHandler(ulong SpeedKbps)
        {
            Device.BeginInvokeOnMainThread(() => {
                MyLabel.Text = string.Format("{0} Kbps", SpeedKbps);
                DownloadButton.IsEnabled = true;
            });
        }
    }
}
