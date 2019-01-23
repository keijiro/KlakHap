using System;
using System.Runtime.InteropServices;

namespace Klak.Hap
{
    internal sealed class ReadBuffer : IDisposable
    {
        #region Initialization/finalization

        public IntPtr PluginPointer { get { return _plugin; } }
        public int Index { get; set; }
        public float Time { get; set; }

        #endregion

        #region Initialization/finalization

        public ReadBuffer()
        {
            _plugin = KlakHap_CreateReadBuffer();
            Index = Int32.MaxValue;
            Time = Single.MaxValue;
        }

        public void Dispose()
        {
            KlakHap_DestroyReadBuffer(_plugin);
        }

        #endregion

        #region Private members

        IntPtr _plugin;

        #endregion

        #region Native plugin entry points

        [DllImport("KlakHap")]
        internal static extern IntPtr KlakHap_CreateReadBuffer();

        [DllImport("KlakHap")]
        internal static extern void KlakHap_DestroyReadBuffer(IntPtr buffer);

        #endregion
    }
}
