
// The CalculateDiffuseChannel function samples from the CastingTexturesArray, index 0, which is the diffuse map bound to this evaluation shader. 
// The function also blends the result with the color value from the CustomBuffer buffer, which is a custom field in the scene (added by the
// scene descriptor .xml file) and which has a 20-byte struct per-material. The function uses the built-in sg_MaterialIdFilter value to do
// the lookup into the buffer.

float4 CalculateDiffuseChannel()
{
	// sample the texture from the CastingTexturesArray[0] using the bound DiffuseSampler sampler
	float4 colorValue = CastingTexturesArray[0].SampleLevel( DiffuseSampler , TexCoord , 0 ) * 0.7f;

	// blend with the custom value from the CustomBuffer, which is of type customStruct, and is defined in "HeaderCode.hlsl"
	colorValue += CustomBuffer[sg_MaterialIdFilter].color * 0.3f;
	
	// return the blended color value
	return colorValue;
}

// The CalculateNormalsChannel calculates the per-texel normals of the output tangent-space normal map. It starts by sampling the input 
// tangent-space normal map of the input geometry, and transforms the normal into object-space coordinates. It then uses the generated 
// destination tangent basis vectors to transform the normal vector into the output tangent-space.

float4 CalculateNormalsChannel()
{
	// sample the input tangent-space texture, and decode into [-1 -> 1] basis
	float3 tangentSpaceNormal = (CastingTexturesArray[1].SampleLevel( NormalSampler , TexCoord , 0 ).xyz * 2.0) - 1.0;
	
	// transform into an object-space vector
	float3 objectSpaceNormal = 	tangentSpaceNormal.x * normalize(Tangent) +
								tangentSpaceNormal.y * normalize(Bitangent) +
								tangentSpaceNormal.z * normalize(Normal);
	
	// transform the object-space vector into the destination tangent space 
	tangentSpaceNormal.x = dot( objectSpaceNormal , normalize(sg_DestinationTangent) );
	tangentSpaceNormal.y = dot( objectSpaceNormal , normalize(sg_DestinationBitangent) );
	tangentSpaceNormal.z = dot( objectSpaceNormal , normalize(sg_DestinationNormal) );
	
	// normalize, the tangent basis is not necessarily orthogonal 
	tangentSpaceNormal = normalize(tangentSpaceNormal);
	
	// encode into [0 -> 1] basis and return
	return float4( ((tangentSpaceNormal + 1.0)/2.0) , 1.0); 
}