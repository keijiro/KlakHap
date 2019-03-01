Shader "Klak/HAP"
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
        return tex2D(_MainTex, input.texcoord);
    }

    ENDCG

    SubShader
    {
        Tags { "RenderType"="Opaque" }
        Pass
        {
            CGPROGRAM
            #pragma vertex Vertex
            #pragma fragment Fragment
            #pragma multi_compile_instancing
            ENDCG
        }
    }
}
