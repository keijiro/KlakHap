Shader "Klak/HAP Q"
{
    Properties
    {
        _MainTex("Texture", 2D) = "white" {}
    }

    CGINCLUDE

    #include "UnityCG.cginc"

    sampler2D _MainTex;
    float4 _MainTex_ST;

    half3 CoCgSY2RGB(half4 i)
    {
    #if !defined(UNITY_COLORSPACE_GAMMA)
        i.xyz = LinearToGammaSpace(i.xyz);
    #endif
        i.xy -= half2(0.50196078431373, 0.50196078431373);
        half s = 1 / ((i.z * (255.0 / 8)) + 1);
        half3 rgb = half3(i.x - i.y, i.y, -i.x - i.y) * s + i.w;
    #if !defined(UNITY_COLORSPACE_GAMMA)
        rgb = GammaToLinearSpace(rgb);
    #endif
        return rgb;
    }

    float4 Vertex(
        float4 position : POSITION,
        inout float2 texcoord : TEXCOORD
    ) : SV_Position
    {
        texcoord = TRANSFORM_TEX(texcoord, _MainTex);
        texcoord.y = 1 - texcoord.y;
        return UnityObjectToClipPos(position);
    }

    fixed4 Fragment(
        float4 position : POSITION,
        float2 texcoord : TEXCOORD
    ) : SV_Target
    {
        return fixed4(CoCgSY2RGB(tex2D(_MainTex, texcoord)), 1);
    }

    ENDCG

    SubShader
    {
        Tags { "RenderType"="Opaque" }
        Pass
        {
            CGPROGRAM
            #pragma multi_compile _ UNITY_COLORSPACE_GAMMA
            #pragma vertex Vertex
            #pragma fragment Fragment
            ENDCG
        }
    }
}
