using UnityEngine;

namespace Klak.Hap
{
    internal static class Utility
    {
        public static void Destroy(Object o)
        {
            if (o == null) return;
            if (Application.isPlaying)
                Object.Destroy(o);
            else
                Object.DestroyImmediate(o);
        }

        public static CodecType DetermineCodecType(int videoType)
        {
            switch (videoType & 0xf)
            {
                case 0xb: return CodecType.Hap;
                case 0xe: return CodecType.HapAlpha;
                case 0xf: return CodecType.HapQ;
            }
            return CodecType.Unsupported;
        }

        public static TextureFormat DetermineTextureFormat(int videoType)
        {
            switch (videoType & 0xf)
            {
                case 0xb: return TextureFormat.DXT1;
                case 0xe: return TextureFormat.DXT5;
                case 0xf: return TextureFormat.DXT5;
                case 0xc: return TextureFormat.BC7;
                case 0x1: return TextureFormat.BC4;
            }
            return TextureFormat.DXT1;
        }

        public static Shader DetermineBlitShader(int videoType)
        {
            if ((videoType & 0xf) == 0xf)
                return Shader.Find("Klak/HAP Q");
            else
                return Shader.Find("Klak/HAP");
        }
    }
}
