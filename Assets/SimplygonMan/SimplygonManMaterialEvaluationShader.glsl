vec4 Diffuse()
{
	vec4 diffuseColor = texture(DiffuseTexture,TEXCOORD0);
	return diffuseColor;
}