
class cResource {

public:
	std::string getcwd();
	std::vector<std::string> directoryList(const std::string& pPath, const std::string& pExtension);


	tSharedBuffer FileRead(const std::string& pFile);
	std::string	FileReadStr(const std::string& pFile);

	bool FileSave(const std::string& pFile, const std::string& pData);

};