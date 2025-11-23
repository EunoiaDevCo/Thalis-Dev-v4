#include "Parser.h"
#include "Program.h"
#include "Tokenizer.h"
#include "Class.h"
#include "ASTExpression.h"
#include <iostream>
#include "Modules/IOModule.h"
#include "Modules/ModuleID.h"

#define COMPILE_ERROR(Line, Column, Msg, Ret) { std::cout << Line << "(" << Column << ") " << Msg << std::endl; return Ret; } 

Parser::Parser(Program* program) :
	m_Program(program),
	m_CurrentFunctionReturnsReference(false)
{
}

static char* ReadFileIntoMemoryNullTerminate(const char* path)
{
	FILE* file = fopen(path, "rb");
	fseek(file, 0, SEEK_END);
	size_t fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* fileContents = (char*)malloc(fileSize + 1);
	fread(fileContents, 1, fileSize, file);
	fileContents[fileSize] = 0;
	fclose(file);
	return fileContents;
}

void Parser::Parse(const std::string& path)
{
	char* contents = ReadFileIntoMemoryNullTerminate(path.c_str());
	Tokenizer tokenizer;
	tokenizer.at = contents;

	Token token = tokenizer.GetToken();

	while (token.type != TokenTypeT::END)
	{
		if (token.type == TokenTypeT::IMPORT)
		{
			ParseImport(&tokenizer);
		}
		else if (token.type == TokenTypeT::CLASS)
		{
			ParseClass(&tokenizer);
		}

		token = tokenizer.GetToken();
	}

	m_Program->Resolve();
	m_Program->EmitCode();

	free(contents);
}

bool Parser::ParseImport(Tokenizer* tokenizer)
{
	Token token = tokenizer->GetToken();
	if (token.type == TokenTypeT::IDENTIFIER) //Built in module
	{
		std::string builtInModule(token.text, token.length);

		if (m_Program->GetModuleID(builtInModule) != INVALID_ID)
		{
			tokenizer->Expect(TokenTypeT::SEMICOLON);
			return true;
		}

		if (builtInModule == "IO") { m_Program->AddModule("IO", IO_MODULE_ID); IOModule::Init(); }

		tokenizer->Expect(TokenTypeT::SEMICOLON);
	}
	else if (token.type == TokenTypeT::STRING_LITERAL) //User defined module
	{
		std::string path(token.text, token.length);
		
	}

	return true;
}

static void SkipStatement(Tokenizer* tokenizer)
{
	int braceDepth = 0;

	while (true)
	{
		Token t = tokenizer->GetToken();
		if (t.type == TokenTypeT::END)
			break;

		if (t.type == TokenTypeT::OPEN_BRACE)
			braceDepth++;
		else if (t.type == TokenTypeT::CLOSE_BRACE)
		{
			braceDepth--;
			if (braceDepth == 0)
				break;
		}
		else if (t.type == TokenTypeT::SEMICOLON && braceDepth == 0)
			break;
	}
}

bool Parser::ParseClass(Tokenizer* tokenizer)
{
	Token nameToken;
	if (tokenizer->Expect(TokenTypeT::IDENTIFIER, &nameToken)) COMPILE_ERROR(nameToken.line, nameToken.column, "Expected identifier after class", false);
	std::string className(nameToken.text, nameToken.length);

	m_CurrentClassName = className;

	Token openBrace;
	if (tokenizer->Expect(TokenTypeT::OPEN_BRACE, &openBrace)) COMPILE_ERROR(nameToken.line, nameToken.column, "Expected '{' after class name", false);

	Class* cls = new Class(className);
	uint16 id = m_Program->AddClass(cls);
	//------------------------------------------------------------
	// Pass 1: Parse all class variables (fields + static)
	//------------------------------------------------------------
	{
		Token savePos = openBrace; // remember where we started

		uint64 memberOffset = 0;
		uint64 staticOffset = 0;
		while (true)
		{
			Token t = tokenizer->PeekToken();
			if (t.type == TokenTypeT::CLOSE_BRACE || t.type == TokenTypeT::END)
				break;

			if (ParseClassVariable(tokenizer, cls, &memberOffset, &staticOffset))
				continue;

			// Skip over functions or anything we don't understand
			SkipStatement(tokenizer);
		}

		// rewind tokenizer to start of class body for second pass
		tokenizer->SetPeek(savePos);
		cls->SetSize(memberOffset);
		cls->SetStaticDataSize(staticOffset);
	}

	//------------------------------------------------------------
	// Pass 2: Parse all functions
	//------------------------------------------------------------
	bool firstIter = true;
	while (true)
	{
		Token t = firstIter ? tokenizer->GetToken() : tokenizer->PeekToken();
		firstIter = false;
		if (t.type == TokenTypeT::CLOSE_BRACE) break;
		if (t.type == TokenTypeT::END) return false;

		if (ParseFunction(tokenizer, cls))
		{
			continue;
		}

		//SkipStatement(tokenizer);
		break;
	}

	if (!cls->HasAssignSTFunction())
	{
		cls->AddFunction(GenerateDefaultCopyFunction(cls, "operator="));
	}

	if (!cls->HasCopyConstructor())
	{
		cls->AddFunction(GenerateDefaultCopyFunction(cls, className));
	}
}

static AccessModifier ParseAccessModifier(Tokenizer* tokenizer)
{
	Token accessModifier = tokenizer->PeekToken();
	if (accessModifier.type == TokenTypeT::PUBLIC)
	{
		tokenizer->GetToken();
		return AccessModifier::PUBLIC;
	}
	else if (accessModifier.type == TokenTypeT::PRIVATE)
	{
		tokenizer->GetToken();
		return AccessModifier::PRIVATE;
	}
	else
	{
		return AccessModifier::PUBLIC;
	}
}


bool Parser::ParseFunction(Tokenizer* tokenizer, Class* cls)
{
	Token t = tokenizer->GetToken();
	Function* function = new Function();

	function->accessModifier = AccessModifier::PUBLIC;
	Token prev = tokenizer->PeekToken();
	if (t.type == TokenTypeT::PUBLIC)
	{
		function->accessModifier = AccessModifier::PUBLIC;
		t = tokenizer->GetToken();
	}
	else if (t.type == TokenTypeT::PRIVATE)
	{
		function->accessModifier = AccessModifier::PRIVATE;
		t = tokenizer->GetToken();
	}

	bool isDestructor = false;
	if (t.type == TokenTypeT::TILDE)
	{
		isDestructor = true;
		t = tokenizer->GetToken();
	}

	// ---- Parse static keyword ----
	function->isStatic = false;
	if (t.type == TokenTypeT::STATIC)
	{
		function->isStatic = true;
	}
	else if (t.type == TokenTypeT::VIRTUAL)
	{
		function->isVirtual = true;
	}

	// ---- Detect constructor ----
	bool isConstructor = false;
	if (!function->isStatic && !isDestructor)
	{
		Token peek = tokenizer->PeekToken();

		if (std::string(t.text, t.length) == m_CurrentClassName &&
			peek.type == TokenTypeT::OPEN_PAREN) // constructor has no return type
		{
			isConstructor = true;
		}
	}

	if (isConstructor)
	{
		function->returnInfo.type = (uint16)ValueType::VOID_T;
		function->name = m_CurrentClassName;
	}
	else if (isDestructor)
	{
		function->returnInfo.type = (uint16)ValueType::VOID_T;
		function->name = "~" + m_CurrentClassName;
	}
	else
	{
		// ---- Normal function: parse return type + name ----
		if (function->isStatic || function->isVirtual)
			t = tokenizer->GetToken();
		else
			t = prev;

		function->returnInfo.type = ParseType(t);

		uint8 pointerLevel = ParsePointerLevel(tokenizer);
		function->returnInfo.pointerLevel = pointerLevel;

		bool returnsReference = false;
		Token referenceToken = tokenizer->PeekToken();
		if (referenceToken.type == TokenTypeT::AND)
		{
			tokenizer->Expect(TokenTypeT::AND);
			returnsReference = true;
		}

		m_CurrentFunctionReturnsReference = returnsReference;
		function->returnsReference = returnsReference;

		t = tokenizer->GetToken();
		if (t.type == TokenTypeT::OPERATOR)
		{
			t = tokenizer->GetToken();
			if (t.type == TokenTypeT::EQUALS) function->name = "operator=";
			else if (t.type == TokenTypeT::PLUS) function->name = "operator+";
			else if (t.type == TokenTypeT::MINUS) function->name = "operator-";
			else if (t.type == TokenTypeT::ASTERISK) function->name = "operator*";
			else if (t.type == TokenTypeT::SLASH) function->name = "operator/";
			else if (t.type == TokenTypeT::MOD) function->name = "operator%";
			else if (t.type == TokenTypeT::EQUALS_EQUALS) function->name = "operator==";
			else if (t.type == TokenTypeT::NOT_EQUAL) function->name = "operator!=";
			else if (t.type == TokenTypeT::LESS) function->name = "operator<";
			else if (t.type == TokenTypeT::GREATER) function->name = "operator>";
			else if (t.type == TokenTypeT::LESS_EQUALS) function->name = "operator<=";
			else if (t.type == TokenTypeT::GREATER_EQUALS) function->name = "operator>=";
		}
		else if (t.type == TokenTypeT::IDENTIFIER)
		{
			function->name = std::string(t.text, t.length);
		}
		else
		{
			delete function;
			return false;
		}
	}

	if (function->name == "Main")
	{
		m_Program->SetClassWithMainFunction(m_Program->GetClassID(m_CurrentClassName));
	}

	Scope* functionScope = new Scope();
	m_ScopeStack.push_back(functionScope);

	if (tokenizer->Expect(TokenTypeT::OPEN_PAREN, &t))
	{
		return false;
	}

	while (true)
	{
		FunctionParameter param;
		param.type.pointerLevel = 0;

		Token typeToken = tokenizer->GetToken();
		if (typeToken.type == TokenTypeT::CLOSE_PAREN)
			break;

		param.type.type = ParseType(typeToken);
		param.type.pointerLevel = ParsePointerLevel(tokenizer);

		param.isReference = false;
		Token peek = tokenizer->PeekToken();
		if (peek.type == TokenTypeT::AND)
		{
			tokenizer->Expect(TokenTypeT::AND);
			param.isReference = true;
		}

		t = tokenizer->GetToken();
		if (t.type != TokenTypeT::IDENTIFIER)
		{
			delete function;
			return false;
		}

		std::string paramName(t.text, t.length);
		param.variableID = functionScope->AddLocal(paramName, param.type);

		function->parameters.push_back(param);

		t = tokenizer->GetToken();
		if (t.type == TokenTypeT::COMMA) continue;
		else if (t.type == TokenTypeT::CLOSE_PAREN) break;
		else
		{
			return false;
		}
	}

	if (tokenizer->Expect(TokenTypeT::OPEN_BRACE))
	{
		delete function; return false;
	}

	while (true)
	{
		Token peek = tokenizer->PeekToken();
		if (peek.type == TokenTypeT::CLOSE_BRACE)
		{
			tokenizer->Expect(TokenTypeT::CLOSE_BRACE);
			break;
		}

		if (!ParseStatement(function, tokenizer))
		{
			delete function;
			return false;
		}
	}

	function->numLocals = functionScope->GetNumLocals();

	m_ScopeStack.pop_back();
	delete functionScope;

	cls->AddFunction(function);
	return true;
}

bool Parser::ParseClassVariable(Tokenizer* tokenizer, Class* cls, uint64* memberOffset, uint64* staticOffset)
{
	AccessModifier accessModifier = ParseAccessModifier(tokenizer);

	bool isStatic = false;
	if (tokenizer->PeekToken().type == TokenTypeT::STATIC)
	{
		tokenizer->Expect(TokenTypeT::STATIC);
		isStatic = true;
	}

	Token typeToken = tokenizer->GetToken();
	std::string typeName(typeToken.text, typeToken.length);
	uint16 type = ParseType(typeToken);

	if (type == INVALID_ID) return false;

	uint64 typeSize = m_Program->GetTypeSize(type);
	uint8 pointerLevel = ParsePointerLevel(tokenizer);
	if (pointerLevel > 0) typeSize = sizeof(void*);

	Token nameToken;
	if (tokenizer->Expect(TokenTypeT::IDENTIFIER, &nameToken)) return false;
	std::string name(nameToken.text, nameToken.length);

	std::vector<uint32> arrayDimensions;
	Token openBracket = tokenizer->PeekToken();
	if (openBracket.type == TokenTypeT::OPEN_BRACKET)
	{
		ParseArrayDimensions(tokenizer, arrayDimensions);
		for (uint32 i = 0; i < arrayDimensions.size(); i++)
			typeSize *= arrayDimensions[i];

		typeSize += sizeof(ArrayHeader);
	}

	Token equals = tokenizer->PeekToken();
	ASTExpression* initializeExpr = nullptr;

	if (equals.type == TokenTypeT::EQUALS)
	{
		tokenizer->GetToken();
		initializeExpr = ParseExpression(tokenizer);
	}

	if (tokenizer->Expect(TokenTypeT::SEMICOLON)) return false;

	if (isStatic)
	{
		uint64 finalOffset = *staticOffset;
		if (!arrayDimensions.empty())
			finalOffset += sizeof(ArrayHeader);

		cls->AddStaticField(name, type, pointerLevel, finalOffset, typeSize, arrayDimensions.data(), arrayDimensions.size(), initializeExpr);
		*staticOffset += typeSize;
	}
	else
	{
		uint64 finalOffset = *memberOffset;
		if (!arrayDimensions.empty())
			finalOffset += sizeof(ArrayHeader);

		cls->AddMemberField(name, type, pointerLevel, finalOffset, typeSize, arrayDimensions.data(), arrayDimensions.size());
		*memberOffset += typeSize;
	}

	return true;
}

uint16 Parser::ParseType(const Token& t)
{
	switch (t.type)
	{
	case TokenTypeT::IDENTIFIER: {
		std::string className(t.text, t.length);
		return m_Program->GetClassID(className);
	} break;

	case TokenTypeT::VOID_T: return (uint16)ValueType::VOID_T;
	case TokenTypeT::BOOL: return (uint16)ValueType::BOOL;
	case TokenTypeT::CHAR: return (uint16)ValueType::CHAR;
	case TokenTypeT::UINT8: return (uint16)ValueType::UINT8;
	case TokenTypeT::UINT16: return (uint16)ValueType::UINT16;
	case TokenTypeT::UINT32: return (uint16)ValueType::UINT32;
	case TokenTypeT::UINT64: return (uint16)ValueType::UINT64;
	case TokenTypeT::INT8: return (uint16)ValueType::INT8;
	case TokenTypeT::INT16: return (uint16)ValueType::INT16;
	case TokenTypeT::INT32: return (uint16)ValueType::INT32;
	case TokenTypeT::INT64: return (uint16)ValueType::INT64;
	case TokenTypeT::REAL32: return (uint16)ValueType::REAL32;
	case TokenTypeT::REAL64: return (uint16)ValueType::REAL64;
	default: return INVALID_ID;
	}
}

uint8 Parser::ParsePointerLevel(Tokenizer* tokenizer)
{
	Token pointer = tokenizer->PeekToken();
	uint8 pointerLevel = 0;
	while (pointer.type == TokenTypeT::ASTERISK)
	{
		pointerLevel++;
		tokenizer->Expect(TokenTypeT::ASTERISK);
		pointer = tokenizer->PeekToken();
	}

	return pointerLevel;
}

bool Parser::ParseStatement(Function* function, Tokenizer* tokenizer)
{
	Token t = tokenizer->GetToken();

	bool declaringPrimitive = false;
	ValueType primitiveType = ValueType::VOID_T;
	if (t.type == TokenTypeT::IDENTIFIER)
	{
		std::string identifier(t.text, t.length);
		Token next = tokenizer->GetToken();
		if (next.type == TokenTypeT::IDENTIFIER) //Declaring user defined type
		{
			uint16 type = m_Program->GetClassID(identifier);
			std::string name(next.text, next.length);
			uint16 slot = m_ScopeStack.back()->AddLocal(name, TypeInfo(type, 0));

			next = tokenizer->GetToken();
			if (next.type == TokenTypeT::SEMICOLON)
			{
				std::vector<ASTExpression*> argExprs(0);
				ASTExpressionDeclareObjectWithConstructor* declareObjectExpr = new ASTExpressionDeclareObjectWithConstructor(type, argExprs, slot);
				function->body.push_back(declareObjectExpr);
				return true;
			}
			else if (next.type == TokenTypeT::OPEN_PAREN)
			{
				std::vector<ASTExpression*> argExprs;
				ParseArguments(tokenizer, argExprs);
				if (tokenizer->Expect(TokenTypeT::SEMICOLON)) return false;
				ASTExpressionDeclareObjectWithConstructor* declareObjectExpr = new ASTExpressionDeclareObjectWithConstructor(type, argExprs, slot);
				function->body.push_back(declareObjectExpr);
				return true;
			}
			else if (next.type == TokenTypeT::EQUALS)
			{
				ASTExpression* assignExpr = ParseExpression(tokenizer);
				if (tokenizer->Expect(TokenTypeT::SEMICOLON)) return false;
				ASTExpressionDeclareObjectWithAssign* declareObjectExpr = new ASTExpressionDeclareObjectWithAssign(type, slot, assignExpr);
				function->body.push_back(declareObjectExpr);
				return true;
			}
		}
		else if (next.type == TokenTypeT::ASTERISK)
		{
			uint8 pointerLevel = ParsePointerLevel(tokenizer) + 1;
			bool isReference = false;
			Token referenceToken = tokenizer->PeekToken();
			if (referenceToken.type == TokenTypeT::AND)
			{
				tokenizer->Expect(TokenTypeT::AND);
				isReference = true;
			}
			uint16 type = m_Program->GetClassID(identifier);
			Token nameToken;
			if (tokenizer->Expect(TokenTypeT::IDENTIFIER, &nameToken))return false;
			std::string name(nameToken.text, nameToken.length);
			uint16 slot = m_ScopeStack.back()->AddLocal(name, TypeInfo(type, pointerLevel));

			next = tokenizer->GetToken();
			ASTExpression* assignExpr = nullptr;
			if (next.type == TokenTypeT::SEMICOLON)
			{
				assignExpr = nullptr;
			}
			else if (next.type == TokenTypeT::EQUALS)
			{
				assignExpr = ParseExpression(tokenizer);
				if (tokenizer->Expect(TokenTypeT::SEMICOLON)) return false;
			}
			else if (next.type == TokenTypeT::OPEN_BRACKET)
			{
				tokenizer->SetPeek(next);
				std::vector<uint32> arrayDimensions;
				ParseArrayDimensions(tokenizer, arrayDimensions);
				Token peek = tokenizer->PeekToken();
				std::vector<ASTExpression*> initializeExprs;
				if (peek.type == TokenTypeT::EQUALS)
				{
					tokenizer->Expect(TokenTypeT::EQUALS);
					ParseArrayInitializers(tokenizer, initializeExprs);
				}

				if (tokenizer->Expect(TokenTypeT::SEMICOLON, &peek)) COMPILE_ERROR(peek.line, peek.column, "Expected ';' in array declaration", false);

				ASTExpressionStackArrayDeclare* declareArrayExpr = new ASTExpressionStackArrayDeclare(type, pointerLevel, slot, arrayDimensions, initializeExprs);
				function->body.push_back(declareArrayExpr);
				return true;
			}
			else
			{
				return false;
			}

			if (isReference)
			{
				if (assignExpr == nullptr) COMPILE_ERROR(next.line, next.column, "Declared reference requires an assign value", false);
				ASTExpressionDeclareReference* declareReferenceExpr = new ASTExpressionDeclareReference(type, pointerLevel, assignExpr, slot);
				function->body.push_back(declareReferenceExpr);
				return true;
			}
			else
			{
				ASTExpressionDeclarePointer* declarePointerExpr = new ASTExpressionDeclarePointer(type, pointerLevel, slot, assignExpr);
				function->body.push_back(declarePointerExpr);
				return true;
			}
		}
		else if (next.type == TokenTypeT::AND)
		{
			uint16 type = m_Program->GetClassID(identifier);
			Token nameToken;
			if (tokenizer->Expect(TokenTypeT::IDENTIFIER, &nameToken))return false;
			std::string name(nameToken.text, nameToken.length);
			uint16 slot = m_ScopeStack.back()->AddLocal(name, TypeInfo(type, 0));

			Token equalsToken;
			if (tokenizer->Expect(TokenTypeT::EQUALS, &equalsToken)) COMPILE_ERROR(equalsToken.line, equalsToken.column, "Expected '=' in reference declaration", false);
			ASTExpression* assignExpr = ParseExpression(tokenizer);

			Token semicolon;
			if (tokenizer->Expect(TokenTypeT::SEMICOLON, &semicolon)) COMPILE_ERROR(semicolon.line, semicolon.column, "Expected ';' after reference declaration", false);

			ASTExpressionDeclareReference* declareReferenceExpr = new ASTExpressionDeclareReference(type, 0, assignExpr, slot);
			function->body.push_back(declareReferenceExpr);
			return true;
		}
		else
		{
			tokenizer->SetPeek(t);
			ASTExpression* expr = ParseExpression(tokenizer);
			if(expr->setIsStatement)
				expr->isStatement = true;
			tokenizer->Expect(TokenTypeT::SEMICOLON);
			function->body.push_back(expr);
			return true;
		}
	}
	else if (t.type == TokenTypeT::IF)
	{
		if (tokenizer->Expect(TokenTypeT::OPEN_PAREN)) return false;
		ASTExpression* conditionExpr = ParseExpression(tokenizer);
		if (tokenizer->Expect(TokenTypeT::CLOSE_PAREN)) return false;

		bool pushIfScope = false;
		bool pushElseScope = false;
		std::vector<ASTExpression*> ifExprs;
		std::vector<ASTExpression*> elseExprs;

		Token next = tokenizer->PeekToken();
		if (next.type == TokenTypeT::OPEN_BRACE)
		{
			tokenizer->Expect(TokenTypeT::OPEN_BRACE);
			pushIfScope = true;
			Scope* scope = new Scope(m_ScopeStack.back());
			m_ScopeStack.push_back(scope);
			while (true)
			{
				Token peekBody = tokenizer->PeekToken();
				if (peekBody.type == TokenTypeT::CLOSE_BRACE) { tokenizer->GetToken(); break; }
				if (!ParseStatement(function, tokenizer)) return false;
				ASTExpression* statement = function->body.back();
				function->body.pop_back();
				ifExprs.push_back(statement);
			}

			delete scope;
			m_ScopeStack.pop_back();
		}
		else
		{
			if (!ParseStatement(function, tokenizer))
				return false;

			ASTExpression* statement = function->body.back();
			function->body.pop_back();
			ifExprs.push_back(statement);
		}

		Token maybeElse = tokenizer->PeekToken();
		if (maybeElse.type == TokenTypeT::ELSE)
		{
			tokenizer->GetToken();
			Token braceElse = tokenizer->PeekToken();
			if (braceElse.type == TokenTypeT::OPEN_BRACE)
			{
				tokenizer->GetToken();
				pushElseScope = true;
				Scope* scope = new Scope(m_ScopeStack.back());
				m_ScopeStack.push_back(scope);
				while (true)
				{
					Token peekBody = tokenizer->PeekToken();
					if (peekBody.type == TokenTypeT::CLOSE_BRACE) { tokenizer->GetToken(); break; }
					if (!ParseStatement(function, tokenizer)) return false;
					ASTExpression* statement = function->body.back();
					function->body.pop_back();
					elseExprs.push_back(statement);
				}

				delete scope;
				m_ScopeStack.pop_back();
			}
			else
			{
				if (!ParseStatement(function, tokenizer)) return false;
				ASTExpression* statement = function->body.back();
				function->body.pop_back();
				elseExprs.push_back(statement);
			}
		}

		ASTExpressionIfElse* ifElseExpr = new ASTExpressionIfElse(conditionExpr, pushIfScope, pushElseScope, ifExprs, elseExprs);
		function->body.push_back(ifElseExpr);
		return true;
	}
	else if (t.type == TokenTypeT::FOR)
	{
		if (tokenizer->Expect(TokenTypeT::OPEN_PAREN)) return false;

		ASTExpression* declareExpr = nullptr;
		ASTExpression* conditionExpr = nullptr;
		ASTExpression* incrementExpr = nullptr;
		std::vector<ASTExpression*> body;

		// init
		Token peek = tokenizer->PeekToken();
		if (peek.type != TokenTypeT::SEMICOLON)
		{
			if (!ParseStatement(function, tokenizer)) return false;
			declareExpr = function->body.back();
			function->body.pop_back();
		}
		else
		{
			tokenizer->GetToken();
		}

		// condition
		peek = tokenizer->PeekToken();
		if (peek.type != TokenTypeT::SEMICOLON)
			conditionExpr = ParseExpression(tokenizer);

		if (tokenizer->Expect(TokenTypeT::SEMICOLON)) return false;

		// increment
		peek = tokenizer->PeekToken();
		if (peek.type != TokenTypeT::CLOSE_PAREN)
		{
			if (!ParseStatement(function, tokenizer)) return false;
			incrementExpr = function->body.back();
			function->body.pop_back();
		}

		// body
		Token brace = tokenizer->PeekToken();
		if (brace.type == TokenTypeT::OPEN_BRACE)
		{
			Scope* forScope = new Scope(m_ScopeStack.back());
			m_ScopeStack.push_back(forScope);

			tokenizer->Expect(TokenTypeT::OPEN_BRACE);
			while (true)
			{
				Token peekBody = tokenizer->PeekToken();
				if (peekBody.type == TokenTypeT::CLOSE_BRACE) { tokenizer->GetToken(); break; }
				if (!ParseStatement(function, tokenizer)) return false;
				ASTExpression* statement = function->body.back();
				function->body.pop_back();
				body.push_back(statement);
			}

			delete forScope;
			m_ScopeStack.pop_back();
		}
		else
		{
			if (!ParseStatement(function, tokenizer)) return false;
			ASTExpression* statement = function->body.back();
			function->body.pop_back();
			body.push_back(statement);
		}

		ASTExpressionFor* forExpr = new ASTExpressionFor(declareExpr, conditionExpr, incrementExpr, body);
		function->body.push_back(forExpr);
		return true;
	}
	else if (t.type == TokenTypeT::WHILE)
	{
		std::vector<ASTExpression*> body;

		if (tokenizer->Expect(TokenTypeT::OPEN_PAREN)) return false;
		ASTExpression* conditionExpr = ParseExpression(tokenizer);
		if (tokenizer->Expect(TokenTypeT::CLOSE_PAREN)) return false;

		Token brace = tokenizer->PeekToken();
		if (brace.type == TokenTypeT::OPEN_BRACE)
		{
			tokenizer->Expect(TokenTypeT::OPEN_BRACE);

			Scope* scope = new Scope(m_ScopeStack.back());
			m_ScopeStack.push_back(scope);

			while (true)
			{
				Token peekBody = tokenizer->PeekToken();
				if (peekBody.type == TokenTypeT::CLOSE_BRACE) { tokenizer->GetToken(); break; }
				if (!ParseStatement(function, tokenizer)) return false;
				ASTExpression* statement = function->body.back();
				function->body.pop_back();
				body.push_back(statement);
			}

			delete scope;
			m_ScopeStack.pop_back();
		}
		else
		{
			if (!ParseStatement(function, tokenizer)) return false;
			ASTExpression* statement = function->body.back();
			function->body.pop_back();
			body.push_back(statement);
		}

		ASTExpressionWhile* whileExpr = new ASTExpressionWhile(conditionExpr, body);
		function->body.push_back(whileExpr);
		return true;
	}
	else if (t.type == TokenTypeT::BREAK)
	{
		Token semicolon;
		if (tokenizer->Expect(TokenTypeT::SEMICOLON, &semicolon)) COMPILE_ERROR(semicolon.line, semicolon.column, "Expected '{' after break", false);
		ASTExpressionBreak* breakExpr = new ASTExpressionBreak();
		function->body.push_back(breakExpr);
		return true;
	}
	else if (t.type == TokenTypeT::CONTINUE)
	{
		Token semicolon;
		if (tokenizer->Expect(TokenTypeT::SEMICOLON, &semicolon)) COMPILE_ERROR(semicolon.line, semicolon.column, "Expected '{' after break", false);
		ASTExpressionContinue* continueExpr = new ASTExpressionContinue();
		function->body.push_back(continueExpr);
		return true;
	}
	else if (t.type == TokenTypeT::RETURN)
	{
		Token peek = tokenizer->PeekToken();
		if (peek.type == TokenTypeT::SEMICOLON)
		{
			tokenizer->Expect(TokenTypeT::SEMICOLON);
			ASTExpression* returnExpr = new ASTExpressionReturn(nullptr);
			function->body.push_back(returnExpr);
			return true;
		}
		else
		{
			ASTExpression* expr = ParseExpression(tokenizer);
			Token semicolon;
			if (tokenizer->Expect(TokenTypeT::SEMICOLON, &semicolon)) COMPILE_ERROR(semicolon.line, semicolon.column, "Expected ';' after return expression", false);
			ASTExpressionReturn* returnExpr = new ASTExpressionReturn(expr, m_CurrentFunctionReturnsReference);
			function->body.push_back(returnExpr);
			return true;
		}
	}
	else if (t.type == TokenTypeT::DELETE_T)
	{
		Token peek = tokenizer->PeekToken();
		bool deleteArray = false;
		if (peek.type == TokenTypeT::OPEN_BRACKET)
		{
			tokenizer->Expect(TokenTypeT::OPEN_BRACKET);
			if (tokenizer->Expect(TokenTypeT::CLOSE_BRACKET, &peek)) COMPILE_ERROR(peek.line, peek.column, "Expected ']' in delete[]", false);
			deleteArray = true;
		}

		ASTExpression* expr = ParseExpression(tokenizer);
		Token semicolon;
		if (tokenizer->Expect(TokenTypeT::SEMICOLON, &semicolon)) COMPILE_ERROR(semicolon.line, semicolon.column, "Expected ';' after delete expression", false);
		
		ASTExpressionDelete* deleteExpr = new ASTExpressionDelete(expr, deleteArray);
		function->body.push_back(deleteExpr);
		return true;
	}
	else if (t.type == TokenTypeT::UINT8)
	{
		declaringPrimitive = true;
		primitiveType = ValueType::UINT8;
	}
	else if (t.type == TokenTypeT::UINT16)
	{
		declaringPrimitive = true;
		primitiveType = ValueType::UINT16;
	}
	else if (t.type == TokenTypeT::UINT32)
	{
		declaringPrimitive = true;
		primitiveType = ValueType::UINT32;
	}
	else if (t.type == TokenTypeT::UINT64)
	{
		declaringPrimitive = true;
		primitiveType = ValueType::UINT64;
	}
	else if (t.type == TokenTypeT::INT8)
	{
		declaringPrimitive = true;
		primitiveType = ValueType::INT8;
	}
	else if (t.type == TokenTypeT::INT16)
	{
		declaringPrimitive = true;
		primitiveType = ValueType::INT16;
	}
	else if (t.type == TokenTypeT::INT32)
	{
		declaringPrimitive = true;
		primitiveType = ValueType::INT32;
	}
	else if (t.type == TokenTypeT::INT64)
	{
		declaringPrimitive = true;
		primitiveType = ValueType::INT64;
	}
	else if (t.type == TokenTypeT::REAL32)
	{
		declaringPrimitive = true;
		primitiveType = ValueType::REAL32;
	}
	else if (t.type == TokenTypeT::REAL64)
	{
		declaringPrimitive = true;
		primitiveType = ValueType::REAL64;
	}
	else if (t.type == TokenTypeT::CHAR)
	{
		declaringPrimitive = true;
		primitiveType = ValueType::CHAR;
	}
	else if (t.type == TokenTypeT::BOOL)
	{
		declaringPrimitive = true;
		primitiveType = ValueType::BOOL;
	}
	else if (t.type == TokenTypeT::VOID_T)
	{
		declaringPrimitive = true;
		primitiveType = ValueType::VOID_T;
	}
	else
	{
		tokenizer->SetPeek(t);
		ASTExpression* expr = ParseExpression(tokenizer);
		if(expr->setIsStatement)
			expr->isStatement = true;
		tokenizer->Expect(TokenTypeT::SEMICOLON);
		function->body.push_back(expr);
		return true;
	}

	if (declaringPrimitive)
	{
		uint8 pointerLevel = ParsePointerLevel(tokenizer);
		Token referenceToken = tokenizer->PeekToken();
		bool isReference = false;
		if (referenceToken.type == TokenTypeT::AND)
		{
			tokenizer->Expect(TokenTypeT::AND);
			isReference = true;
		}

		Token nameToken;
		if (tokenizer->Expect(TokenTypeT::IDENTIFIER, &nameToken)) return false;
		std::string name(nameToken.text, nameToken.length);

		std::vector<uint32> dimensions;
		ParseArrayDimensions(tokenizer, dimensions);

		uint16 slot = m_ScopeStack.back()->AddLocal(name, TypeInfo((uint16)primitiveType, pointerLevel));

		if (!dimensions.empty())
		{
			if (isReference)
			{
				COMPILE_ERROR(t.line, t.column, "Cannot declare array of references", false);
			}

			Token peek = tokenizer->PeekToken();
			std::vector<ASTExpression*> initializeExprs;
			if (peek.type == TokenTypeT::EQUALS)
			{
				tokenizer->Expect(TokenTypeT::EQUALS);
				ParseArrayInitializers(tokenizer, initializeExprs);
			}

			Token semicolon;
			if (tokenizer->Expect(TokenTypeT::SEMICOLON, &semicolon)) COMPILE_ERROR(semicolon.line, semicolon.column, "Expected ';' after array declaration", nullptr);

			ASTExpressionStackArrayDeclare* arrayDeclare = new ASTExpressionStackArrayDeclare((uint16)primitiveType, pointerLevel, slot, dimensions, initializeExprs);
			function->body.push_back(arrayDeclare);
			return true;
		}

		Token peek = tokenizer->PeekToken();
		ASTExpression* assignExpr = nullptr;
		if (peek.type == TokenTypeT::EQUALS)
		{
			tokenizer->Expect(TokenTypeT::EQUALS);
			assignExpr = ParseExpression(tokenizer);
		}
		else
		{
			if (isReference)
			{
				COMPILE_ERROR(t.line, t.column, "Declared reference requires an assign value", false);
			}
		}

		if (tokenizer->Expect(TokenTypeT::SEMICOLON)) return false;

		if (isReference)
		{
			ASTExpressionDeclareReference* declareReferenceExpr = new ASTExpressionDeclareReference((uint16)primitiveType, pointerLevel, assignExpr, slot);
			function->body.push_back(declareReferenceExpr);
			return true;
		}
		else if (pointerLevel > 0)
		{
			ASTExpressionDeclarePointer* declarePointerExpr = new ASTExpressionDeclarePointer((uint16)primitiveType, pointerLevel, slot, assignExpr);
			function->body.push_back(declarePointerExpr);
			return true;
		}
		else
		{
			ASTExpressionDeclarePrimitive* declarePrimitiveExpr = new ASTExpressionDeclarePrimitive(primitiveType, slot, assignExpr);
			function->body.push_back(declarePrimitiveExpr);
			return true;
		}
	}

	return false;
}

ASTExpression* Parser::ParseExpression(Tokenizer* tokenizer)
{
	ASTExpression* lhs = ParseUnary(tokenizer);
	if (!lhs) return nullptr;
	return ParseBinaryOpRHS(0, lhs, tokenizer);
}

ASTExpression* Parser::ParseUnary(Tokenizer* tokenizer)
{
	Token tok = tokenizer->PeekToken();

	switch (tok.type)
	{
	case TokenTypeT::ASTERISK: { // dereference
		tokenizer->Expect(TokenTypeT::ASTERISK);
		ASTExpression* expr = ParseExpression(tokenizer);
		ASTExpressionDereference* dereferenceExpr = new ASTExpressionDereference(expr);
		return dereferenceExpr;
	} break;
	case TokenTypeT::AND: { // address-of
		tokenizer->Expect(TokenTypeT::AND);
		ASTExpression* expr = ParseExpression(tokenizer);
		ASTExpressionAddressOf* addressOfExpr = new ASTExpressionAddressOf(expr);
		return addressOfExpr;
	} break;
	case TokenTypeT::PLUS_PLUS: {
		tokenizer->Expect(TokenTypeT::PLUS_PLUS);
		ASTExpression* expr = ParseExpression(tokenizer);
		ASTExpressionUnaryUpdate* unaryExpr = new ASTExpressionUnaryUpdate(expr, ASTUnaryUpdateOp::PRE_INC);
		return unaryExpr;
	} break;
	case TokenTypeT::MINUS_MINUS: {
		tokenizer->Expect(TokenTypeT::MINUS_MINUS);
		ASTExpression* expr = ParseExpression(tokenizer);
		ASTExpressionUnaryUpdate* unaryExpr = new ASTExpressionUnaryUpdate(expr, ASTUnaryUpdateOp::PRE_DEC);
		return unaryExpr;
	} break;
	case TokenTypeT::NOT: { // logical not
		tokenizer->Expect(TokenTypeT::NOT);
		
	} break;
	case TokenTypeT::MINUS: { // unary negation
		tokenizer->Expect(TokenTypeT::MINUS);
		
	} break;
	default:
		return ParsePostFix(tokenizer);
	}

	return ParsePostFix(tokenizer);
}

static int32 GetPrecedence(const Token& token)
{
	switch (token.type)
	{
	case TokenTypeT::ASTERISK:
	case TokenTypeT::SLASH:
	case TokenTypeT::MOD:
		return 20;

	case TokenTypeT::PLUS:
	case TokenTypeT::MINUS:
		return 10;

	case TokenTypeT::BITSHIFT_LEFT:
	case TokenTypeT::BITSHIFT_RIGHT:
		return 9;

	case TokenTypeT::LESS:
	case TokenTypeT::LESS_EQUALS:
	case TokenTypeT::GREATER:
	case TokenTypeT::GREATER_EQUALS:
		return 8;

	case TokenTypeT::EQUALS_EQUALS:
	case TokenTypeT::NOT_EQUAL:
		return 7;

	case TokenTypeT::AND:
		return 6;

	case TokenTypeT::PIPE:
		return 5;

	case TokenTypeT::LOGICAL_AND:
		return 3;

	case TokenTypeT::LOGICAL_OR:
		return 2;

	default:
		return -1;
	}
}

ASTExpression* Parser::ParseBinaryOpRHS(int32 exprPrec, ASTExpression* lhs, Tokenizer* tokenizer)
{
	while (true)
	{
		Token opToken = tokenizer->PeekToken();  // lookahead, dont consume yet
		int tokenPrec = GetPrecedence(opToken);

		if (tokenPrec < exprPrec)
			return lhs;

		// Now we know it's a valid operator, consume it
		tokenizer->GetToken();

		ASTExpression* rhs = ParseUnary(tokenizer);
		if (!rhs) return nullptr;

		// If the next operator has higher precedence, handle it first
		Token nextOp = tokenizer->PeekToken();
		int nextPrec = GetPrecedence(nextOp);

		if (tokenPrec < nextPrec)
		{
			rhs = ParseBinaryOpRHS(tokenPrec + 1, rhs, tokenizer);
			if (!rhs) return nullptr;
		}

		Operator opEnum;
		switch (opToken.type)
		{
		case TokenTypeT::PLUS:          opEnum = Operator::ADD; break;
		case TokenTypeT::MINUS:         opEnum = Operator::MINUS; break;
		case TokenTypeT::ASTERISK:      opEnum = Operator::MULTIPLY; break;
		case TokenTypeT::SLASH:         opEnum = Operator::DIVIDE; break;
		case TokenTypeT::MOD:		   opEnum = Operator::MOD; break;

		case TokenTypeT::LESS:          opEnum = Operator::LESS; break;
		case TokenTypeT::LESS_EQUALS:    opEnum = Operator::LESS_EQUALS; break;
		case TokenTypeT::GREATER:       opEnum = Operator::GREATER; break;
		case TokenTypeT::GREATER_EQUALS: opEnum = Operator::GREATER_EQUALS; break;
		case TokenTypeT::EQUALS_EQUALS:   opEnum = Operator::EQUALS; break;
		case TokenTypeT::NOT_EQUAL:    opEnum = Operator::NOT_EQUALS; break;

		case TokenTypeT::LOGICAL_AND: opEnum = Operator::LOGICAL_AND; break;
		case TokenTypeT::LOGICAL_OR: opEnum = Operator::LOGICAL_OR; break;

		case TokenTypeT::AND: opEnum = Operator::BITWISE_AND; break;
		case TokenTypeT::PIPE: opEnum = Operator::BITWISE_OR; break;
		case TokenTypeT::BITSHIFT_LEFT: opEnum = Operator::BITSHIFT_LEFT; break;
		case TokenTypeT::BITSHIFT_RIGHT: opEnum = Operator::BITSHIFT_RIGHT; break;

		default: return lhs; // shouldnt happen
		}

		lhs = new ASTExpressionBinary(lhs, rhs, opEnum);
	}
}

ASTExpression* Parser::ParsePostFix(Tokenizer* tokenizer)
{
	ASTExpression* expr = ParsePrimary(tokenizer);

	while (true)
	{
		Token tok = tokenizer->PeekToken();

		if (tok.type == TokenTypeT::PLUS_PLUS)
		{
			tokenizer->GetToken();
			ASTExpressionUnaryUpdate* unaryExpr = new ASTExpressionUnaryUpdate(expr, ASTUnaryUpdateOp::POST_INC);
			return unaryExpr;
		}
		else if (tok.type == TokenTypeT::MINUS_MINUS)
		{
			tokenizer->GetToken();
			ASTExpressionUnaryUpdate* unaryExpr = new ASTExpressionUnaryUpdate(expr, ASTUnaryUpdateOp::POST_DEC);
			return unaryExpr;
		}
		else if (tok.type == TokenTypeT::PLUS_EQUALS)
		{
			tokenizer->GetToken();
			ASTExpression* amountExpr = ParseExpression(tokenizer);
		}
		else if (tok.type == TokenTypeT::MINUS_EQUALS)
		{
			tokenizer->GetToken();
			ASTExpression* amountExpr = ParseExpression(tokenizer);
		}
		else if (tok.type == TokenTypeT::TIMES_EQUALS)
		{
			tokenizer->GetToken();
			ASTExpression* amountExpr = ParseExpression(tokenizer);
		}
		else if (tok.type == TokenTypeT::DIVIDE_EQUALS)
		{
			tokenizer->GetToken();
			ASTExpression* amountExpr = ParseExpression(tokenizer);
		}
		else if (tok.type == TokenTypeT::MOD_EQUALS)
		{
			tokenizer->GetToken();
			ASTExpression* amountExpr = ParseExpression(tokenizer);
		}
		else
		{
			break;
		}
	}

	return expr;
}

ASTExpression* Parser::ParsePrimary(Tokenizer* tokenizer)
{
	Token t = tokenizer->GetToken();

	if (t.type == TokenTypeT::NUMBER_LITERAL)
	{
		std::string numStr(t.text, t.length);
		if (numStr.find('.') != std::string::npos)
		{
			ASTExpressionLiteral* literalExpr = new ASTExpressionLiteral(Value::MakeReal64(std::stod(numStr), m_Program->GetInitializationAllocator()));
			return literalExpr;
		}
		else
		{
			ASTExpressionLiteral* literalExpr = new ASTExpressionLiteral(Value::MakeInt64(std::stoll(numStr), m_Program->GetInitializationAllocator()));
			return literalExpr;
		}
	}
	else if (t.type == TokenTypeT::STRING_LITERAL)
	{
		std::string str(t.text, t.length);
		for (size_t i = 0; i < str.size(); ++i)
		{
			if (str[i] == '\\' && i + 1 < str.size())
			{
				switch (str[i + 1])
				{
				case 'n':  str.replace(i, 2, "\n");  break;
				case 't':  str.replace(i, 2, "\t");  break;
				case 'r':  str.replace(i, 2, "\r");  break;
				case '\\': str.replace(i, 2, "\\");  break;
				case '"':  str.replace(i, 2, "\"");  break;
				default: continue;
				}
			}
		}
		Value literal = Value::MakeCStr(str, m_Program->GetHeapAllocator());
		m_Program->AddToStringPool((char*)literal.data);
		ASTExpressionLiteral* literalExpr = new ASTExpressionLiteral(literal);
		return literalExpr;
	}
	else if (t.type == TokenTypeT::CHAR_LITERAL)
	{
		char c;

		if (t.length == 1)
		{
			c = t.text[0];
		}
		else if (t.length == 2 && t.text[0] == '\\')
		{
			switch (t.text[1])
			{
			case 'n':  c = '\n'; break;
			case 't':  c = '\t'; break;
			case 'r':  c = '\r'; break;
			case '\\': c = '\\'; break;
			case '\'': c = '\''; break;
			case '"':  c = '"';  break;
			default:   c = t.text[1]; break; // unknown escape, use raw
			}
		}
		else
		{
			// fallback -> invalid char literal
			c = '?';
		}

		ASTExpressionLiteral* literalExpr = new ASTExpressionLiteral(Value::MakeChar(c, m_Program->GetInitializationAllocator()));

		return literalExpr;
	}
	else if (t.type == TokenTypeT::TRUE_T)
	{
		ASTExpressionLiteral* literalExpr = new ASTExpressionLiteral(Value::MakeBool(true, m_Program->GetInitializationAllocator()));
		return literalExpr;
	}
	else if (t.type == TokenTypeT::FALSE_T)
	{
		ASTExpressionLiteral* literalExpr = new ASTExpressionLiteral(Value::MakeBool(false, m_Program->GetInitializationAllocator()));
		return literalExpr;
	}
	else if (t.type == TokenTypeT::NULLPTR)
	{
		ASTExpressionLiteral* literalExpr = new ASTExpressionLiteral(Value::MakeNULL());
		return literalExpr;
	}
	else if (t.type == TokenTypeT::OPEN_PAREN)
	{
		ASTExpression* expr = ParseExpression(tokenizer);
		tokenizer->Expect(TokenTypeT::CLOSE_PAREN);
		return expr;
	}
	else if (t.type == TokenTypeT::NEW)
	{
		Token typeToken;
		if (tokenizer->Expect(TokenTypeT::IDENTIFIER, &typeToken)) COMPILE_ERROR(typeToken.line, typeToken.column, "Expected identifier after 'new'", nullptr);
		uint16 type = ParseType(typeToken);
		uint8 pointerLevel = ParsePointerLevel(tokenizer);

		Token peek = tokenizer->PeekToken();
		if (peek.type == TokenTypeT::OPEN_BRACKET)
		{
			tokenizer->Expect(TokenTypeT::OPEN_BRACKET);
			ASTExpression* sizeExpr = ParseExpression(tokenizer);
			tokenizer->Expect(TokenTypeT::CLOSE_BRACKET);

			ASTExpressionNewArray* newArrayExpr = new ASTExpressionNewArray(type, pointerLevel, sizeExpr);
			return newArrayExpr;
		}
		else if (peek.type == TokenTypeT::OPEN_PAREN)
		{
			tokenizer->Expect(TokenTypeT::OPEN_PAREN);
			std::vector<ASTExpression*> argExprs;
			ParseArguments(tokenizer, argExprs);
			ASTExpressionNew* newExpr = new ASTExpressionNew(type, argExprs);
			return newExpr;
		}
		else
		{
			COMPILE_ERROR(peek.line, peek.column, "Expected '[' or '(' after identifer", nullptr);
		}
	}
	else if (t.type == TokenTypeT::IDENTIFIER || t.type == TokenTypeT::THIS)
	{
		Token next = tokenizer->GetToken();

		if (t.type == TokenTypeT::THIS && next.type != TokenTypeT::ARROW)
		{
			tokenizer->SetPeek(next);
			ASTExpressionThis* thisExpr = new ASTExpressionThis(m_Program->GetClassID(m_CurrentClassName));
			return thisExpr;
		}

		if (next.type == TokenTypeT::OPEN_PAREN) //Function call
		{
			std::vector<ASTExpression*> argExprs;
			ParseArguments(tokenizer, argExprs);

			std::string functionName(t.text, t.length);
			uint16 classID = m_Program->GetClassID(m_CurrentClassName);
			Class* constructorClass = m_Program->GetClassByName(functionName);
			if (constructorClass)
			{
				ASTExpressionConstructorCall* constructorCallExpr = new ASTExpressionConstructorCall(m_Program->GetClassID(functionName), argExprs);
				return constructorCallExpr;
			}

			ASTExpressionStaticFunctionCall* functionCallExpr = new ASTExpressionStaticFunctionCall(classID, functionName, argExprs);
			return functionCallExpr;
		}
		else if (next.type == TokenTypeT::DOT)
		{
			tokenizer->SetPeek(next);
			std::vector<std::pair<std::string, bool>> members;
			members.push_back(std::make_pair(std::string(t.text, t.length), false));
			bool functionCall = false;
			ParseMembers(tokenizer, members, &functionCall);
			return ParseExpressionChain(tokenizer, nullptr, members, functionCall);
		}
		else if (next.type == TokenTypeT::ARROW)
		{
			tokenizer->SetPeek(next);
			std::vector<std::pair<std::string, bool>> members;
			members.push_back(std::make_pair(std::string(t.text, t.length), true));
			bool functionCall = false;
			ParseMembers(tokenizer, members, &functionCall);
			return ParseExpressionChain(tokenizer, nullptr, members, functionCall);
		}
		else if (next.type == TokenTypeT::EQUALS)
		{
			ASTExpression* assignExpr = ParseExpression(tokenizer);

			std::string variableName(t.text, t.length);
			uint16 slot = m_ScopeStack.back()->Resolve(variableName);
			if (slot == INVALID_ID)
			{
				std::vector<std::string> members;
				members.push_back(variableName);
				uint16 thisClassID = m_Program->GetClassID(m_CurrentClassName);

				Class* thisClass = m_Program->GetClass(thisClassID);
				TypeInfo typeInfo;
				bool isArray = false;
				uint64 offset = thisClass->CalculateStaticOffset(m_Program, members, &typeInfo, &isArray);
				
				if (offset == UINT64_MAX)
				{
					ASTExpressionThis* thisExpr = new ASTExpressionThis(thisClassID);
					ASTExpression* dereferenceThis = new ASTExpressionDereference(thisExpr);
					ASTExpressionPushMember* pushMemberExpr = new ASTExpressionPushMember(dereferenceThis, members);
					ASTExpressionSet* setExpr = new ASTExpressionSet(pushMemberExpr, assignExpr);
					return setExpr;
				}

				ASTExpressionStaticVariable* staticVariableExpr = new ASTExpressionStaticVariable(thisClassID, offset, typeInfo, isArray);
				ASTExpressionSet* setExpr = new ASTExpressionSet(staticVariableExpr, assignExpr);
				return setExpr;
				
			}
			else
			{
				TypeInfo typeInfo = m_ScopeStack.back()->GetLocalTypeInfo(slot);
				ASTExpressionPushLocal* localExpr = new ASTExpressionPushLocal(slot, typeInfo);
				ASTExpressionSet* setExpr = new ASTExpressionSet(localExpr, assignExpr);
				return setExpr;
			}
		}
		else if (next.type == TokenTypeT::OPEN_BRACKET)
		{
			std::vector<ASTExpression*> indexExprs;
			ParseArrayIndices(tokenizer, indexExprs);

			Token peek = tokenizer->PeekToken();
			ASTExpression* assignExpr = nullptr;
			if (peek.type == TokenTypeT::EQUALS)
			{
				tokenizer->Expect(TokenTypeT::EQUALS);
				assignExpr = ParseExpression(tokenizer);
			}

			std::string variableName(t.text, t.length);
			uint16 slot = m_ScopeStack.back()->Resolve(variableName);
			TypeInfo typeInfo = m_ScopeStack.back()->GetLocalTypeInfo(slot);

			ASTExpression* expr = nullptr;
			if (slot == INVALID_ID)
			{
				return nullptr;
			}
			else
			{
				ASTExpressionPushLocal* variableExpr = new ASTExpressionPushLocal(slot, typeInfo);
				ASTExpressionPushIndex* indexExpr = new ASTExpressionPushIndex(variableExpr, indexExprs);
				if (assignExpr)
				{
					expr = new ASTExpressionSet(indexExpr, assignExpr);
				}
				else
				{
					expr = indexExpr;
				}
			}

			peek = tokenizer->PeekToken();
			if (peek.type == TokenTypeT::DOT || peek.type == TokenTypeT::ARROW)
			{
				std::vector<std::pair<std::string, bool>> members;
				bool functionCall = false;
				ParseMembers(tokenizer, members, &functionCall);
				expr = ParseExpressionChain(tokenizer, expr, members, functionCall);
				expr->setIsStatement = false;
			}
		
			return expr;
		}
		else
		{
			std::string variableName(t.text, t.length);
			Scope* scope = m_ScopeStack.back();
			uint16 slot = m_ScopeStack.back()->Resolve(variableName);
			TypeInfo typeInfo = m_ScopeStack.back()->GetLocalTypeInfo(slot);
			tokenizer->SetPeek(next);
			if (slot == INVALID_ID)
			{
				std::vector<std::string> members;
				members.push_back(variableName);
				uint16 thisClassID = m_Program->GetClassID(m_CurrentClassName);
				Class* cls = m_Program->GetClass(thisClassID);
				TypeInfo staticTypeInfo;
				bool isArray = false;
				uint64 staticOffset = cls->CalculateStaticOffset(m_Program, members, &staticTypeInfo, &isArray);
				if (staticOffset == UINT64_MAX)
				{
					ASTExpressionThis* thisExpr = new ASTExpressionThis(thisClassID);
					ASTExpressionDereference* dereferenceThisExpr = new ASTExpressionDereference(thisExpr);
					ASTExpressionPushMember* pushMemberExpr = new ASTExpressionPushMember(dereferenceThisExpr, members);
					return pushMemberExpr;
				}
				else
				{
					ASTExpressionStaticVariable* staticVariableExpr = new ASTExpressionStaticVariable(thisClassID, staticOffset, staticTypeInfo, isArray);
					return staticVariableExpr;
				}
			}
			else
			{
				ASTExpressionPushLocal* localExpr = new ASTExpressionPushLocal(slot, typeInfo);
				return localExpr;
			}
		}
	}
}

void Parser::ParseArguments(Tokenizer* tokenizer, std::vector<ASTExpression*>& args)
{
	while (true)
	{
		Token peek = tokenizer->PeekToken();
		if (peek.type == TokenTypeT::CLOSE_PAREN)
		{
			tokenizer->GetToken();
			break;
		}

		ASTExpression* argExpr = ParseExpression(tokenizer);
		args.push_back(argExpr);
		Token next = tokenizer->GetToken();
		if (next.type == TokenTypeT::COMMA) continue;
		if (next.type == TokenTypeT::CLOSE_PAREN) break;
	}
}

void Parser::ParseArrayDimensions(Tokenizer* tokenizer, std::vector<uint32>& dimensions)
{
	Token openBracket = tokenizer->PeekToken();
	while (openBracket.type == TokenTypeT::OPEN_BRACKET)
	{
		tokenizer->Expect(TokenTypeT::OPEN_BRACKET);
		Token numberLiteral;
		if (tokenizer->Expect(TokenTypeT::NUMBER_LITERAL, &numberLiteral)) COMPILE_ERROR(numberLiteral.line, numberLiteral.column, "Expected integer literal for array dimension");
		uint32 dimension = std::stol(std::string(numberLiteral.text, numberLiteral.length));
		dimensions.push_back(dimension);
		Token closeBracket;
		if (tokenizer->Expect(TokenTypeT::CLOSE_BRACKET, &closeBracket)) COMPILE_ERROR(closeBracket.line, closeBracket.column, "Exprected ']' after integer literal");
		openBracket = tokenizer->PeekToken();
	}
}

void Parser::ParseArrayInitializers(Tokenizer* tokenizer, std::vector<ASTExpression*>& initializers)
{
	Token openBrace;
	if (tokenizer->Expect(TokenTypeT::OPEN_BRACE, &openBrace)) COMPILE_ERROR(openBrace.line, openBrace.column, "Expected array initializer '{'");

	while (true)
	{
		Token peek = tokenizer->PeekToken();
		if (peek.type == TokenTypeT::OPEN_BRACE)
		{
			ParseArrayInitializers(tokenizer, initializers);
		}
		else if (peek.type == TokenTypeT::CLOSE_BRACE)
		{
			break;
		}
		else
		{
			ASTExpression* initializeExpr = ParseExpression(tokenizer);
			initializers.push_back(initializeExpr);
		}

		Token comma = tokenizer->GetToken();
		if (comma.type == TokenTypeT::CLOSE_BRACE) break;
		if (comma.type == TokenTypeT::COMMA) continue;
		else COMPILE_ERROR(comma.line, comma.column, "Expected ',' or '}' in array initializer");
	}
}

void Parser::ParseArrayIndices(Tokenizer* tokenizer, std::vector<ASTExpression*>& indexExprs)
{
	while (true)
	{
		ASTExpression* indexExpr = ParseExpression(tokenizer);
		indexExprs.push_back(indexExpr);

		Token closeBracket;
		if (tokenizer->Expect(TokenTypeT::CLOSE_BRACKET, &closeBracket)) COMPILE_ERROR(closeBracket.line, closeBracket.column, "Expected ']' after array index");
		Token peek = tokenizer->PeekToken();
		if (peek.type == TokenTypeT::OPEN_BRACKET)
		{
			tokenizer->Expect(TokenTypeT::OPEN_BRACKET);
		}
		else break;
	}
}

ASTExpression* Parser::ParseExpressionChain(Tokenizer* tokenizer, ASTExpression* objExpr, const std::vector<std::pair<std::string, bool>>& members, bool functionCall)
{
	Token peek = tokenizer->PeekToken();

	std::vector<ASTExpression*> arrayIndices;
	if (peek.type == TokenTypeT::OPEN_BRACKET)
	{
		tokenizer->Expect(TokenTypeT::OPEN_BRACKET);
		ParseArrayIndices(tokenizer, arrayIndices);
	}

	ASTExpression* chainExpr = objExpr;
	for (uint32 i = 0; i < members.size(); i++)
	{
		std::string memberName = members[i].first;
		bool isArrow = members[i].second;

		uint16 moduleID = m_Program->GetModuleID(memberName);
		uint16 classID = m_Program->GetClassID(memberName);

		if (moduleID != INVALID_ID) //Module constant or function
		{
			std::string identifier = members[++i].first;
			Token next = tokenizer->GetToken();
			if (next.type == TokenTypeT::OPEN_PAREN)
			{
				std::vector<ASTExpression*> argExprs;
				ParseArguments(tokenizer, argExprs);
				chainExpr = MakeModuleFunctionCall(moduleID, memberName, identifier, argExprs);
			}
			else
			{
				chainExpr = MakeModuleConstant(moduleID, memberName, identifier);
			}

			return chainExpr;
		}
		else if(classID != INVALID_ID) //Static variable or function
		{
			Token next = tokenizer->GetToken();
			if (next.type == TokenTypeT::OPEN_PAREN && members.size() == 2)
			{
				std::vector<ASTExpression*> argExprs;
				ParseArguments(tokenizer, argExprs);

				uint16 classID = m_Program->GetClassID(members[0].first);
				std::string functionCall = members[++i].first;
				

				chainExpr = new ASTExpressionStaticFunctionCall(classID, functionCall, argExprs);
				Token peek = tokenizer->PeekToken();
				if (peek.type == TokenTypeT::DOT ||
					peek.type == TokenTypeT::ARROW)
				{
					tokenizer->GetToken();
					std::vector<std::pair<std::string, bool>> subMembers;
					bool subFunctionCall = false;
					ParseMembers(tokenizer, subMembers, &subFunctionCall);
					chainExpr = ParseExpressionChain(tokenizer, chainExpr, subMembers, subFunctionCall);
				}
			}
			else
			{
				std::vector<std::string> updatedMembers;
				for (uint32 j = i + 1; j < members.size(); j++)
				{
					updatedMembers.push_back(members[j].first);
					i++;

					if (members[i].second)
					{
						break;
					}
				}

				chainExpr = new ASTExpressionStaticVariable(classID, updatedMembers);
				if (!arrayIndices.empty())
				{
					chainExpr = new ASTExpressionPushIndex(chainExpr, arrayIndices);
				}
				tokenizer->SetPeek(peek);
			}

			continue;
		}

		if (i == 0 && objExpr == nullptr)
		{
			if (members[0].first == "this")
			{
				objExpr = new ASTExpressionThis(m_Program->GetClassID(m_CurrentClassName));
				objExpr = new ASTExpressionDereference(objExpr);
			}
			else
			{
				uint32 slot = m_ScopeStack.back()->Resolve(members[0].first);
				TypeInfo localTypeInfo = m_ScopeStack.back()->GetLocalTypeInfo(slot);
				objExpr = new ASTExpressionPushLocal(slot, localTypeInfo);
				if (members[0].second)
					objExpr = new ASTExpressionDereference(objExpr);
			}
			i++;
		}
		else
		{
			objExpr = chainExpr;
		}

		std::vector<std::string> updatedMembers;
		uint32 count = members.size();
		if (functionCall) count--;

		bool incremented = false;
		for (uint32 j = i; j < count; j++)
		{
			updatedMembers.push_back(members[j].first);
			i++;
			incremented = true;

			if (members[j].second)
				break;
		}

		
		
		if (incremented)
			chainExpr = new ASTExpressionPushMember(objExpr, updatedMembers);
		else
			chainExpr = objExpr;
	}

	if (!arrayIndices.empty())
	{
		chainExpr = new ASTExpressionPushIndex(chainExpr, arrayIndices);
	}

	peek = tokenizer->PeekToken();
	if (peek.type == TokenTypeT::EQUALS)
	{
		tokenizer->Expect(TokenTypeT::EQUALS);
		ASTExpression* assignExpr = ParseExpression(tokenizer);
		ASTExpressionSet* setExpr = new ASTExpressionSet(chainExpr, assignExpr);
		return setExpr;
	}
	else if (peek.type == TokenTypeT::OPEN_PAREN)
	{
		tokenizer->Expect(TokenTypeT::OPEN_PAREN);
		std::vector<ASTExpression*> argExprs;
		ParseArguments(tokenizer, argExprs);

		ASTExpressionMemberFunctionCall* functionCallExpr = new ASTExpressionMemberFunctionCall(chainExpr, members.back().first, argExprs);
		peek = tokenizer->PeekToken();
		chainExpr = functionCallExpr;
		if (peek.type == TokenTypeT::DOT || peek.type == TokenTypeT::ARROW)
		{
			std::vector<std::pair<std::string, bool>> subMembers;
			bool subFunctionCall = false;
			ParseMembers(tokenizer, subMembers, &subFunctionCall);
			chainExpr = ParseExpressionChain(tokenizer, chainExpr, subMembers, subFunctionCall);
		}
		else if (peek.type == TokenTypeT::OPEN_BRACKET)
		{
			tokenizer->Expect(TokenTypeT::OPEN_BRACKET);
			std::vector<ASTExpression*> subIndexExprs;
			ParseArrayIndices(tokenizer, subIndexExprs);
			chainExpr = new ASTExpressionPushIndex(chainExpr, subIndexExprs);

			peek = tokenizer->PeekToken();
			if (peek.type == TokenTypeT::DOT || peek.type == TokenTypeT::ARROW)
			{
				std::vector<std::pair<std::string, bool>> subMembers;
				bool subFunctionCall = false;
				ParseMembers(tokenizer, subMembers, &subFunctionCall);
				chainExpr = ParseExpressionChain(tokenizer, chainExpr, subMembers, subFunctionCall);
			}
		}
		else
		{
			chainExpr->isStatement = true;
		}
	}
	else if (peek.type == TokenTypeT::DOT)
	{
		std::vector<std::pair<std::string, bool>> subMembers;
		bool subFunctionCall = false;
		ParseMembers(tokenizer, subMembers, &subFunctionCall);
		chainExpr = ParseExpressionChain(tokenizer, chainExpr, subMembers, subFunctionCall);
	}
	else if (peek.type == TokenTypeT::ARROW)
	{
		std::vector<std::pair<std::string, bool>> subMembers;
		bool subFunctionCall = false;
		ParseMembers(tokenizer, subMembers, &subFunctionCall);
		chainExpr = ParseExpressionChain(tokenizer, chainExpr, subMembers, subFunctionCall);
	}

	return chainExpr;
}

void Parser::ParseMembers(Tokenizer* tokenizer, std::vector<std::pair<std::string, bool>>& members, bool* functionCall)
{
	Token subMember = tokenizer->PeekToken();
	while (subMember.type == TokenTypeT::DOT || subMember.type == TokenTypeT::ARROW)
	{
		bool isArrow = subMember.type == TokenTypeT::ARROW;
		tokenizer->GetToken();
		subMember = tokenizer->GetToken();
		if (subMember.type != TokenTypeT::IDENTIFIER) return;
		members.push_back(std::make_pair(std::string(subMember.text, subMember.length), isArrow));
		subMember = tokenizer->PeekToken();
	}

	if (subMember.type == TokenTypeT::OPEN_PAREN)
		*functionCall = true;
	else
		*functionCall = false;
}

ASTExpressionModuleFunctionCall* Parser::MakeModuleFunctionCall(uint16 moduleID, const std::string& moduleName, const std::string& functionName, const std::vector<ASTExpression*>& args)
{
	uint16 function = 0;

	if (moduleName == "IO")
	{
		if (functionName == "Println")		function = (uint16)IOModuleFunction::PRINTLN;
		else if (functionName == "Print")	function = (uint16)IOModuleFunction::PRINT;
	}

	return new ASTExpressionModuleFunctionCall(moduleID, function, args);
}

ASTExpressionModuleConstant* Parser::MakeModuleConstant(uint16 moduleID, const std::string& moduleName, const std::string& constantName)
{
	uint16 constant = 0;

	if (moduleName == "IO")
	{

	}

	return new ASTExpressionModuleConstant(moduleID, constant);
}

Function* Parser::GenerateDefaultCopyFunction(Class* cls, const std::string& name)
{
	Function* function = new Function();
	function->accessModifier = AccessModifier::PUBLIC;
	function->isStatic = false;
	function->isVirtual = false;
	function->name = name;
	function->returnInfo = TypeInfo((uint16)ValueType::VOID_T, 0);
	function->numLocals = 1;
	function->returnsReference = false;

	Scope* functionScope = new Scope();

	FunctionParameter parameter;
	parameter.isReference = true;
	parameter.type = TypeInfo(cls->GetID(), 0);
	parameter.variableID = functionScope->AddLocal("#TLS_Other", TypeInfo(cls->GetID(), 0));
	function->parameters.push_back(parameter);

	const std::vector<ClassField>& members = cls->GetMemberFields();
	std::vector<std::string> membs;
	membs.resize(1);
	for (uint32 i = 0; i < members.size(); i++)
	{
		const ClassField& member = members[i];
		membs[0] = member.name;

		if (member.numDimensions > 0)
		{
			std::vector<uint16> indexLocals;
			indexLocals.reserve(member.numDimensions);

			for (uint32 d = 0; d < member.numDimensions; d++)
			{
				std::string idxName = "#TLS_idx_" + std::to_string(i) + "_" + std::to_string(d);
				indexLocals.push_back(functionScope->AddLocal(idxName, TypeInfo((uint16)ValueType::UINT32, 0)));
			}

			ASTExpression* thisMember = new ASTExpressionPushMember(new ASTExpressionDereference(new ASTExpressionThis(cls->GetID())), membs);
			ASTExpression* otherMember = new ASTExpressionPushMember(new ASTExpressionPushLocal(parameter.variableID, parameter.type), membs);

			std::vector<ASTExpression*> indexExprs;
			for (uint32 d = 0; d < member.numDimensions; d++)
			{
				indexExprs.push_back(new ASTExpressionPushLocal(indexLocals[d], TypeInfo((uint16)ValueType::UINT32, 0)));
			}

			thisMember = new ASTExpressionPushIndex(thisMember, indexExprs);
			otherMember = new ASTExpressionPushIndex(otherMember, indexExprs);

			ASTExpressionSet* innerAssign = new ASTExpressionSet(thisMember, otherMember);

			ASTExpression* loopBody = innerAssign;

			for (int32 dim = member.numDimensions - 1; dim >= 0; dim--)
			{
				uint16 idx = indexLocals[dim];

				ASTExpressionDeclarePrimitive* declareExpr = new ASTExpressionDeclarePrimitive(ValueType::UINT32, idx);

				ASTExpressionBinary* condExpr = new ASTExpressionBinary(
					new ASTExpressionPushLocal(idx, TypeInfo((uint16)ValueType::INT32, 0)),
					new ASTExpressionConstUInt32(member.dimensions[dim]),
					Operator::LESS);

				ASTExpressionUnaryUpdate* incrExpr = new ASTExpressionUnaryUpdate(new ASTExpressionPushLocal(idx, TypeInfo((uint16)ValueType::UINT32, 0)), ASTUnaryUpdateOp::PRE_INC);

				std::vector<ASTExpression*> bodyBlock = { loopBody };

				loopBody = new ASTExpressionFor(declareExpr, condExpr, incrExpr, bodyBlock);
			}

			function->body.push_back(loopBody);
		}
		else
		{
			ASTExpressionPushMember* pushMemberExpr = new ASTExpressionPushMember(new ASTExpressionDereference(new ASTExpressionThis(cls->GetID())), membs); //Access this->member
			ASTExpressionPushMember* pushParamExpr = new ASTExpressionPushMember(new ASTExpressionPushLocal(parameter.variableID, parameter.type), membs); //Access param.member
			ASTExpressionSet* setExpr = new ASTExpressionSet(pushMemberExpr, pushParamExpr);
			function->body.push_back(setExpr);
		}
	}

	delete functionScope;
	return function;
}
