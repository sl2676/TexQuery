#ifndef PARSER_STATE_H
#define PARSER_STATE_H

enum class ParserState {
	DefaultState,
	MathModeState,
	EnvironmentState,
	CommandArgumentState,
	VerbatimState,
};

#endif
