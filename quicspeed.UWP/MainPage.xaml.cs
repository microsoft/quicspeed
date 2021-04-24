/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

namespace quicspeed.UWP
{
    public sealed partial class MainPage
    {
        public MainPage()
        {
            this.InitializeComponent();

            LoadApplication(new quicspeed.App());
        }
    }
}
