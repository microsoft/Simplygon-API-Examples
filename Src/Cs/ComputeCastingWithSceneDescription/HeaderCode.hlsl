
// customStruct is the user-defined custom data structure which is bound to the "MaterialColorsCustomField" custom field
// which is defined in the scene. The custom field is 160 bytes and is mapped to a StructuredBuffer of 8 entries, one
// per material. Note that the "multiplier" value is not used in the example, and is there only to create a more complex
// struct than a regular float4, which requires a specific struct definition in the header code.

struct customStruct
{
	float4 color;
	float multiplier;
};
