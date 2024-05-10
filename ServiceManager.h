#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include "CLI_Utils.h"
#include "Services.h"


class ServiceManager final
{
public:
	DWORD start(_In_ const int argc, _In_ const wchar_t* argv[]);

private:
	enum class Services
	{
		printHelp,
		trackFile,
		trackDir,
		undefined
	};

private:
	Services getServiceType() const;
	std::unique_ptr<IService> createService();

private:
	std::shared_ptr<CLI_Utils::CmdLineParser> opts_;	
	Services serviceType_{ Services::printHelp };
};



#endif		//SERVICE_MANAGER_H
