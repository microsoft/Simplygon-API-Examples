vec4 Diffuse()
{
	vec4 diffuseColor = vec4(0); 
	diffuseColor.xyzw += USERPERVERT1;
	diffuseColor.xyzw += USERPERCORN1.xyxy;
	diffuseColor.xyz += TRIINDEX14;

	if( sg_MaterialIdFilter == 5 )
		diffuseColor = textureLod(DiffuseTexture,TEXCOORD0,4);
	
	if( diffuseColor == vec4(0) && sg_MaterialIdFilter > 5 )
		diffuseColor.xyz += CUSTOM23[ sg_MaterialIdFilter ].xyz;
		
	if( diffuseColor == vec4(0) )
		diffuseColor = texture(DiffuseTexture,TEXCOORD0);
		
	return diffuseColor;
}