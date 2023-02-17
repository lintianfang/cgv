
function(add_vscode_launch_json_content LAUNCH_JSON_CONFIG_VAR TARGET_NAME)
	cmake_parse_arguments(
		PARSE_ARGV 2 CGVARG_ "" "WORKING_DIR" "CMD_LINE_ARGS"
	)

	# determine target names
	cgv_get_static_or_exe_name(NAME_STATIC NAME_EXE ${TARGET_NAME} TRUE)

	# compose JSON list for both plugin and executable variants
	# - plugin build, standard VS Code C++ debugging
	set(CONTENT_LOCAL
"		{
			\"name\": \"Debug '${TARGET_NAME}' (cppdbg)\",
			\"type\": \"cppdbg\",
			\"request\": \"launch\",
			\"program\": \"$<TARGET_FILE:cgv_viewer>\",
			\"args\": [\"${CGVARG__CMD_LINE_ARGS}\"],
			\"cwd\": \"${CGVARG__WORKING_DIR}\",
			\"MIMode\": \"gdb\",
			\"setupCommands\": [
			{\n
				\"description\": \"Use pretty printing of variables and containers\",
				\"text\": \"-enable-pretty-printing\",
				\"ignoreFailures\": true
			},
			{
				\"description\": \"Use Intel-style disassembly\",
				\"text\": \"-gdb-set disassembly-flavor intel\",
				\"ignoreFailures\": true
				}
			]
		}"
	)
	# - plugin build, CodeLLDB debugging
	set(CONTENT_LOCAL "${CONTENT_LOCAL},
		{
			\"name\": \"Debug '${TARGET_NAME}' (CodeLLDB)\",
			\"type\": \"lldb\",
			\"request\": \"launch\",
			\"program\": \"$<TARGET_FILE:cgv_viewer>\",
			\"args\": [\"${CGVARG__CMD_LINE_ARGS}\"],
			\"cwd\": \"${CGVARG__WORKING_DIR}\",
		}"
	)
	# - single executable build, standard VS Code C++ debugging
	set(CONTENT_LOCAL "${CONTENT_LOCAL},
		{
			\"name\": \"Debug '${NAME_EXE}' (cppdbg)\",
			\"type\": \"cppdbg\",
			\"request\": \"launch\",
			\"program\": \"$<TARGET_FILE:cgv_viewer>\",
			\"args\": [\"${CGVARG__CMD_LINE_ARGS}\"],
			\"cwd\": \"${CGVARG__WORKING_DIR}\",
			\"MIMode\": \"gdb\",
			\"setupCommands\": [
			{\n
				\"description\": \"Use pretty printing of variables and containers\",
				\"text\": \"-enable-pretty-printing\",
				\"ignoreFailures\": true
			},
			{
				\"description\": \"Use Intel-style disassembly\",
				\"text\": \"-gdb-set disassembly-flavor intel\",
				\"ignoreFailures\": true
				}
			]
		}"
	)
	# - single executable build, CodeLLDB debugging
	set(CONTENT_LOCAL "${CONTENT_LOCAL},
		{
			\"name\": \"Debug '${NAME_EXE}' (CodeLLDB)\",
			\"type\": \"lldb\",
			\"request\": \"launch\",
			\"program\": \"$<TARGET_FILE:cgv_viewer>\",
			\"args\": [\"${CGVARG__CMD_LINE_ARGS}\"],
			\"cwd\": \"${CGVARG__WORKING_DIR}\",
		}"
	)

	# write to target variable
	set(${LAUNCH_JSON_CONFIG_VAR} ${CONTENT_LOCAL} PARENT_SCOPE)
endfunction()

function(set_plugin_execution_params target_name)
	cmake_parse_arguments(
		PARSE_ARGV 1 CGVARG_ "" "ALTERNATIVE_COMMAND" "ARGUMENTS"
	)

	# command name
	if (CGVARG__ALTERNATIVE_COMMAND)
		set_target_properties(${target_name} PROPERTIES VS_DEBUGGER_COMMAND ${CGVARG__ALTERNATIVE_COMMAND})
		set_target_properties(${target_name} PROPERTIES XCODE_SCHEME_EXECUTABLE ${CGVARG__ALTERNATIVE_COMMAND})
	else()
		set_target_properties(${target_name} PROPERTIES VS_DEBUGGER_COMMAND $<TARGET_FILE:cgv_viewer>)
		set_target_properties(${target_name} PROPERTIES XCODE_SCHEME_EXECUTABLE $<TARGET_FILE:cgv_viewer>)
	endif()

	# command arguments
	set_target_properties(${target_name} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "${CGVARG__ARGUMENTS}")
	set_target_properties(${target_name} PROPERTIES XCODE_SCHEME_ARGUMENTS "${CGVARG__ARGUMENTS}")
endfunction()

function(set_plugin_execution_working_dir target_name path)
	set_target_properties(${target_name} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${path}")
	set_target_properties(${target_name} PROPERTIES XCODE_SCHEME_WORKING_DIRECTORY "${path}")
endfunction()
