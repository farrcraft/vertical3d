#ifndef INCLUDED_XML_VALIDATOR
#define INCLUDED_XML_VALIDATOR

namespace XML
{

	typedef enum VCErrorCode
	{  
		RootElementMismatch, 
		ParameterEntityMismatch,
		PEDeclNestingInvalid,
		StandaloneDecl,
		ElementInvalid,
		AttributeValueType,
		ElementDeclNotUnique,
		ParameterEntityNesting,
		MixedContentNotUnique,
		AttributeIDInvalid,
		AttributeMultipleIDs,
		AttributeNoDefault,
		AttributeIDRefInvalid,
		EntityNameInvalid,
		NameTokenInvalid,
		NotationInvalid,
		EnumerationInvalid,
		AttributeRequired,
		AttributeDefaultInvalid,
		AttributeFixedMismatch,
		ExternalEntityUndeclared,
		NotationUndeclared,
		EmptyElementContent,
		MixedContentInvalid
	} VCErrorCode;

	typedef enum WFCErrorCode
	{  
		ParameterEntityError,
		AttributeNotUnique,
		ExternalAttributeEntity,
		LessThanInAttribute,
		EntityUndeclared,
		EntityRecursion,
		ParameterEntityInDTD,
		ElementTypeMismatch,
		CharRefIllegalCharacter
	} WFCErrorCode;

	class Validator
	{
		public:
			Validator();
			virtual ~Validator();

			virtual void validateDTD(const std::wstring & dtd) = 0;
			virtual void validateBody(void) = 0;
	};

}; // end namespace XML

#endif // INCLUDED_XML_VALIDATOR
