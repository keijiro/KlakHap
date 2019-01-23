using UnityEngine;

namespace Klak.Hap
{
    internal static class Utility
    {
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

        public static Shader DetermineShader(int videoType)
        {
            switch (videoType & 0xf)
            {
                case 0xe: return Shader.Find("Klak/HAP Alpha");
                case 0xf: return Shader.Find("Klak/HAP Q");
            }
            return Shader.Find("Klak/HAP");
        }
    }
}
