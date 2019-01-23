Shader "Klak/HAP Alpha"
{
    Properties
    {
        _MainTex("Texture", 2D) = "white" {}
    }

    CGINCLUDE

    #include "UnityCG.cginc"

    sampler2D _MainTex;
    float4 _MainTex_ST;

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
        return tex2D(_MainTex, texcoord);
    }

    ENDCG

    SubShader
    {
        Tags { "RenderType"="Transparent" "Queue"="Transparent" }
        Pass
        {
            ZWrite Off
            Blend SrcAlpha OneMinusSrcAlpha
            CGPROGRAM
            #pragma vertex Vertex
            #pragma fragment Fragment
            ENDCG
        }
    }
}
