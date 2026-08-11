#version 450
#define INVALID_TEXTURE_INDEX 0xFFFFFFFFu
#define INVALID_MATERIAL_INDEX 0xFFFFFFFFu


   struct MeshInstance
    {
        uint meshId;
        uint materialId;
        mat4 modelTransform;
        mat4 normalTransform;           
    };
  struct MaterialTextures
    {
        uint albedo ;
        uint normalMap ;
        uint metallicRoughness ;
        uint occlusionMap ;
        uint emissive ;
      
    };

    struct MaterialParameters
    {

        vec4 albedo;
        float metallic;
        float roughness;
    };

  struct Material
    {
        MaterialTextures textures;
        MaterialParameters parameters;
    };




layout(std430, set=0, binding=3)readonly buffer MeshInstanceBuffer {
 MeshInstance meshInstances[];
};
layout(std430, set=0, binding=4)readonly buffer MaterialBuffer {
 Material materials[];
};


layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragTangent;


layout(set=1, binding=0) uniform texture2D allTextures[];
layout(set=1, binding=1) uniform sampler defaultSampler;




layout(location =0) out vec4 gAlbedo;
layout(location =1) out vec4 gNormal;
layout(location = 2) out vec4 gMaterial;  
layout(location = 3) out vec4 gEmissive;



vec2 encodeNormal(vec3 n) {
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    if (n.z < 0.0)
        return (1.0 - abs(n.yx)) * sign(n.xy) * 0.5 + 0.5;
    return n.xy * 0.5 + 0.5;
}

void main() {
    MeshInstance instance = meshInstances[gl_InstanceIndex];
    vec3 N = normalize(fragNormal);
    vec3 T = normalize(fragTangent.xyz);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N,T) * fragTangent.w;
    mat3 TBN = mat3(T,B,N);
    if(instance.materialId == INVALID_MATERIAL_INDEX){
        gAlbedo = vec4(1.0,0.0,0.0,1.0);
         gNormal = vec4(encodeNormal(fragNormal), 0.0,1.0);
         gMaterial = vec4(1.0, 0.0,0.0, 1.0);
          gEmissive = vec4(vec3(0.0),1.0);

          return;
    }

    Material material = materials[instance.materialId];
     

    if(material.textures.albedo == INVALID_TEXTURE_INDEX){
        gAlbedo = material.parameters.albedo;
    } else {
        gAlbedo = texture(sampler2D(allTextures[material.textures.albedo], defaultSampler), fragUV);

    }

    if(material.textures.normalMap == INVALID_TEXTURE_INDEX){
        gNormal = vec4(encodeNormal(fragNormal), 0.0,1.0);
    } else {

         vec3 tangentNormal = texture(sampler2D(allTextures[material.textures.normalMap], defaultSampler), fragUV).rgb * 2.0 - 1.0;
    tangentNormal = normalize(tangentNormal);
    vec3 worldNormal = normalize(TBN * tangentNormal);
       gNormal = vec4(encodeNormal(worldNormal), 0.0,1.0);
        
    }
    float ao = 1.0;

    if(material.textures.occlusionMap != INVALID_TEXTURE_INDEX){
        ao = texture(sampler2D(allTextures[material.textures.occlusionMap], defaultSampler), fragUV).r;
    }

    if(material.textures.metallicRoughness == INVALID_TEXTURE_INDEX){
         gMaterial = vec4(ao, material.parameters.metallic,material.parameters.roughness, 1.0);
    } else {

        vec3 metallicRoughness = texture(sampler2D(allTextures[material.textures.metallicRoughness], defaultSampler), fragUV).rgb;

        gMaterial = vec4(ao,metallicRoughness.b,metallicRoughness.g,1.0);

        
    }


    if(material.textures.emissive == INVALID_TEXTURE_INDEX){
        gEmissive = vec4(vec3(0.0),1.0);
    } else {
       gEmissive = texture(sampler2D(allTextures[material.textures.emissive], defaultSampler), fragUV);       
    }
  
}