#ifndef CMD_LINE_PARSER_H
#define CMD_LINE_PARSER_H


namespace CLI_Utils
{

	using CmdLineArgs = std::vector<std::wstring>;
	using CmdLineOpts = std::map<std::wstring, CmdLineArgs>;


	class CmdLineParser final
	{
	public:
		CmdLineParser(_In_ const int argc, _In_ const wchar_t* argv[]);

		const CmdLineArgs& getArgs(_In_ const std::wstring& optName) const;
		const CmdLineArgs& getGlobalArgs() const;

		const CmdLineOpts& getAllOpts() const;

		bool optExists(_In_ const std::wstring& optName) const;

		const std::wstring& getOnlyArg(_In_ const std::wstring& optName) const;

		static const std::wstring& getGlobalOptName();

	private:
		void parseCmdLineArgs(_In_ const int argc, _In_ const wchar_t* argv[]);

	private:
		static const std::wstring globalArgsOptName_;

	private:
		CmdLineOpts cmdLineOpts_;

	};

}		//CLI_Utils




#endif		//CMD_LINE_PARSER_H
