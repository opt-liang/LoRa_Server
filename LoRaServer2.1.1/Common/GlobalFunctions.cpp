#include "GlobalData.hpp"

void Global::InitialiseData(void)
{
	Global::genericDatabase.UpdateBuildTime();
	ConfiguredValueBaseType::GlobalInitialise();	//must be initialised before gatewayList

	applicationList.Initialise();	//application list must be reinitialised to add the null application

	InitialiseDataServerSpecific();
}


void Global::DeleteAllData()
{
	applicationList.DeleteAllElements();
	applicationList.Initialise();	//application list must be reinitialised to add the null application

	DeleteAllDataServerSpecific();
}

