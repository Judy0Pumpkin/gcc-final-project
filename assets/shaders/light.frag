#version 430

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec4 FragPosLightSpace;
in vec4 FragPosSpotLightSpace;  

out vec4 color;

uniform sampler2D ourTexture;
uniform sampler2D shadowMap;
uniform sampler2D spotShadowMap; 
uniform vec3 viewPos;

// TODO#3-4: fragment shader

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float reflectivity;
};

struct DirectionLight {
    int enable;
    vec3 direction;
    vec3 lightColor;
};

struct PointLight {
    int enable;
    vec3 position;
    vec3 lightColor;
    float constant;
    float linear;
    float quadratic;
};

struct Spotlight {
    int enable;
    int cutOffEnable;
    vec3 position;
    vec3 direction;
    vec3 lightColor;
    float cutOff;
    float constant;
    float linear;
    float quadratic;
};

uniform Material material;
uniform DirectionLight dl;
uniform PointLight pl;
uniform Spotlight sl;
uniform samplerCube skybox;
uniform samplerCube pointShadowMap;
uniform float pointLightFarPlane;
uniform int shadowEnable; 
uniform int cutOffEnable;

float calculatePointShadow(vec3 fragPos, vec3 lightPos) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    
    float closestDepth = texture(pointShadowMap, fragToLight).r;
    closestDepth *= pointLightFarPlane;
    
    float bias = 0.2;
    return currentDepth - bias > closestDepth ? 1.0 : 0.0;
}


float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, sampler2D shadowMapTex) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0) return 0.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;
    
    float closestDepth = texture(shadowMapTex, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMapTex, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMapTex, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

// Directional Light
vec3 calculateDirectionLight() {
    if (dl.enable == 0) return vec3(0.0);
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-dl.direction);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 ambient = dl.lightColor * material.ambient;
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = dl.lightColor * (diff * material.diffuse);
    
    // 修正後的高光計算
    vec3 reflectDir = reflect(-lightDir, norm);
    // 1. 確保 shininess 至少有一個極小的正值，防止 pow(0,0)
    float safeShininess = max(material.shininess, 0.1); 
    // 2. 限制點積範圍
    float specBase = max(dot(viewDir, reflectDir), 0.0);
    float spec = pow(specBase, safeShininess);

    vec3 specular = dl.lightColor * (spec * material.specular);
    float shadow = 0.0;
    if (shadowEnable == 1) {
        shadow = calculateShadow(FragPosLightSpace, norm, lightDir, shadowMap);
    }
    return ambient + (1.0 - shadow) * (diffuse + specular);
}

// Point Light
vec3 calculatePointLight() {
    if (pl.enable == 0) return vec3(0.0);
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(pl.position - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    float distance = length(pl.position - FragPos);
    float attenuation = 1.0 / (pl.constant + pl.linear * distance + pl.quadratic * (distance * distance));
    
    vec3 ambient = pl.lightColor * material.ambient;
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = pl.lightColor * (diff * material.diffuse);
    
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = pl.lightColor * (spec * material.specular);
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    float shadow = 0.0;
    if (shadowEnable == 1) {
        shadow = calculatePointShadow(FragPos, pl.position);
    }

    return ambient + (1.0 - shadow) * (diffuse + specular);
}

// Spot Light
vec3 calculateSpotLight() {
    if (sl.enable == 0) return vec3(0.0);
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(sl.position - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    float theta = dot(lightDir, normalize(-sl.direction));
    
    float distance = length(sl.position - FragPos);
    float attenuation = 1.0 / (sl.constant + sl.linear * distance + sl.quadratic * (distance * distance));
    
    vec3 ambient = sl.lightColor * material.ambient;
    
    float shadow = 0.0;
    if (shadowEnable == 1) {
        shadow = calculateShadow(FragPosSpotLightSpace, norm, lightDir, spotShadowMap);
    }

    if (theta > sl.cutOff) {
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = sl.lightColor * (diff * material.diffuse);
        
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec3 specular = sl.lightColor * (spec * material.specular);
        
        float intensity;
        float epsilon = 0.05;
        if (cutOffEnable == 1){
            intensity = clamp((theta - sl.cutOff) / epsilon, 0.0, 1.0);
        }
        else {
            intensity = clamp((theta) / epsilon, 0.0, 1.0);
        }
        ambient *= attenuation;
        diffuse *= attenuation * intensity;
        specular *= attenuation * intensity;
        
        return ambient + (1.0 - shadow) * (diffuse + specular);
    } else {
        return ambient * attenuation;
    }
}

void main() {
    vec3 norm = normalize(Normal);
    vec3 result = vec3(0.0);
    
    if (dl.enable == 1) {
        result += calculateDirectionLight();
    }
    if (pl.enable == 1) {
        result += calculatePointLight();
    }
    if (sl.enable == 1) {
        result += calculateSpotLight();
    }
    
    vec4 texColor = texture(ourTexture, TexCoord);
    result *= texColor.rgb;
    
    if (material.reflectivity > 0.0) {
        vec3 I = normalize(FragPos - viewPos);
        vec3 R = reflect(I, norm);
        vec3 reflection = texture(skybox, R).rgb;
        result = mix(result, reflection, material.reflectivity);
    }
    
    color = vec4(result, 1.0);
}