Shader "Klak/HAP Q"
{
    Properties
    {
        _MainTex("Texture", 2D) = "white" {}
    }

    CGINCLUDE

    #include "UnityCG.cginc"

    struct Attributes
    {
        float4 position : POSITION;
        float2 texcoord : TEXCOORD;
        UNITY_VERTEX_INPUT_INSTANCE_ID
    };

    struct Varyings
    {
        float4 position : SV_Position;
        float2 texcoord : TEXCOORD;
    };

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

    Varyings Vertex(Attributes input)
    {
        UNITY_SETUP_INSTANCE_ID(input);
        Varyings output;
        output.position = UnityObjectToClipPos(input.position);
        output.texcoord = TRANSFORM_TEX(input.texcoord, _MainTex);
        output.texcoord.y = 1 - output.texcoord.y;
        return output;
    }

    fixed4 Fragment(Varyings input) : SV_Target
    {
        return fixed4(CoCgSY2RGB(tex2D(_MainTex, input.texcoord)), 1);
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
            #pragma multi_compile_instancing
            ENDCG
        }
    }
}
