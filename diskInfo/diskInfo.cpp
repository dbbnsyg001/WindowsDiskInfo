#define _WIN32_DCOM
#include <iostream>
using namespace std;
#include <comdef.h>
#include <vector>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

BOOL wmi_run();
BOOL wmi_getDriveLetters();
BOOL wmi_close();
vector<string> diskInfoList;
IWbemLocator* pLoc = NULL;
IWbemServices* pSvc = NULL;
std::string Wchar_tToString(wchar_t* wchar)
{
    wchar_t* wText = wchar;
    DWORD dwNum = WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, NULL, 0, NULL, FALSE);// WideCharToMultiByte的运用
    char* psText;  // psText为char*的临时数组，作为赋值给std::string的中间变量
    psText = new char[dwNum];
    WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, psText, dwNum, NULL, FALSE);// WideCharToMultiByte的再次运用
    std::string szDst = psText;// std::string赋值
    delete[]psText;// psText的清除
    return szDst;
}
bool judgeExistDisk(BSTR str)
{
    string temp = Wchar_tToString(str);

    for (size_t i = 0; i < diskInfoList.size(); ++i)
    {
        if (temp == diskInfoList[i])
        {
            return true;
        }
    }

    diskInfoList.push_back(temp);
    return false;
}
int main(int argc, char** argv)
{
    wmi_run();
    wmi_getDriveLetters();
    system("pause");
    wmi_close();
}

//
// Step 1-5 at:
// https://msdn.microsoft.com/en-us/library/aa390423(VS.85).aspx
BOOL wmi_run()
{
    HRESULT hres;
    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);

    if (FAILED(hres))
    {
        cout << "Failed to initialize COM library. Error code = 0x"
             << hex << hres << endl;
        return 1;                  // Program has failed.
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------
    // Note: If you are using Windows 2000, you need to specify -
    // the default authentication credentials for a user by using
    // a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
    // parameter of CoInitializeSecurity ------------------------
    hres = CoInitializeSecurity(
               NULL,
               -1,                          // COM authentication
               NULL,                        // Authentication services
               NULL,                        // Reserved
               RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
               RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
               NULL,                        // Authentication info
               EOAC_NONE,                   // Additional capabilities
               NULL                         // Reserved
           );

    if (FAILED(hres))
    {
        cout << "Failed to initialize security. Error code = 0x"
             << hex << hres << endl;
        CoUninitialize();
        return 1;                    // Program has failed.
    }

    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------
    //IWbemLocator *pLoc = NULL;
    hres = CoCreateInstance(
               CLSID_WbemLocator,
               0,
               CLSCTX_INPROC_SERVER,
               IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres))
    {
        cout << "Failed to create IWbemLocator object."
             << " Err code = 0x"
             << hex << hres << endl;
        CoUninitialize();
        return 1;                 // Program has failed.
    }

    // Step 4: -----------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method
    //IWbemServices *pSvc = NULL;
    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer pSvc
    // to make IWbemServices calls.
    hres = pLoc->ConnectServer(
               _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
               NULL,                    // User name. NULL = current user
               NULL,                    // User password. NULL = current
               0,                       // Locale. NULL indicates current
               NULL,                    // Security flags.
               0,                       // Authority (e.g. Kerberos)
               0,                       // Context object
               &pSvc                    // pointer to IWbemServices proxy
           );

    if (FAILED(hres))
    {
        cout << "Could not connect. Error code = 0x"
             << hex << hres << endl;
        pLoc->Release();
        CoUninitialize();
        return 1;                // Program has failed.
    }

    cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;
    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------
    hres = CoSetProxyBlanket(
               pSvc,                        // Indicates the proxy to set
               RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
               RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
               NULL,                        // Server principal name
               RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
               RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
               NULL,                        // client identity
               EOAC_NONE                    // proxy capabilities
           );

    if (FAILED(hres))
    {
        cout << "Could not set proxy blanket. Error code = 0x"
             << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 1;               // Program has failed.
    }

    return 0;
}

//
// get Drives, logical Drives and Driveletters
BOOL wmi_getDriveLetters()
{
    // Use the IWbemServices pointer to make requests of WMI.
    // Make requests here:
    HRESULT hres;
    IEnumWbemClassObject* pEnumerator = NULL;
    // get localdrives
    hres = pSvc->ExecQuery(
               bstr_t("WQL"),
               bstr_t("SELECT * FROM Win32_DiskDrive"),
               WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
               NULL,
               &pEnumerator);

    if (FAILED(hres))
    {
        cout << "Query for processes failed. "
             << "Error code = 0x"
             << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;               // Program has failed.
    }
    else
    {
        IWbemClassObject* pclsObj;
        ULONG uReturn = 0;

        while (pEnumerator)
        {
            hres = pEnumerator->Next(WBEM_INFINITE, 1,
                                     &pclsObj, &uReturn);

            if (0 == uReturn) break;

            VARIANT vtProp;
            hres = pclsObj->Get(_bstr_t(L"DeviceID"), 0, &vtProp, 0, 0);
            // adjust string
            wstring tmp = vtProp.bstrVal;
            tmp = tmp.substr(4);
            // Size属性可以获取硬盘的大小哦（暂时注释掉啦）
            // 如果需要使用的话，注意先判断vtProp的类型，目前虽然VARIANT结构体内置多种类型，
            // 但目前只支持四种类型：VT_I4、VT_DISPATCH、VT_BSTR、VT_EMPTY
            // 对应获取值得名称：lVal、pdispVal、bstrVal、none
            // 例如：vtProp类型为VT_BSTR，获取值为vtProp.bstrValue
            // 最后注意：获取的值为宽字符，可以通过WideCharToMultiByte转化为多字符
            // pclsObj->Get(L"Size", 0, &vtProp, 0, 0);
            // 打印硬盘型号
            pclsObj->Get(L"Caption", 0, &vtProp, 0, 0);

            if (!(vtProp.vt == VT_EMPTY || vtProp.vt == VT_I4 ||
                  vtProp.vt == VT_DISPATCH))
                printf("硬盘型号:%ls\n", vtProp.bstrVal);

            wstring wstrQuery = L"Associators of {Win32_DiskDrive.DeviceID='\\\\.\\";
            wstrQuery += tmp;
            wstrQuery += L"'} where AssocClass=Win32_DiskDriveToDiskPartition";
            // reference drive to partition
            IEnumWbemClassObject* pEnumerator1 = NULL;
            hres = pSvc->ExecQuery(
                       bstr_t("WQL"),
                       bstr_t(wstrQuery.c_str()),
                       WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                       NULL,
                       &pEnumerator1);

            if (FAILED(hres))
            {
                cout << "Query for processes failed. "
                     << "Error code = 0x"
                     << hex << hres << endl;
                pSvc->Release();
                pLoc->Release();
                CoUninitialize();
                return FALSE;               // Program has failed.
            }
            else
            {
                IWbemClassObject* pclsObj1;
                ULONG uReturn1 = 0;

                while (pEnumerator1)
                {
                    hres = pEnumerator1->Next(WBEM_INFINITE, 1,
                                              &pclsObj1, &uReturn1);

                    if (0 == uReturn1) break;

                    // 此处获取的都是Win32_DiskPartition磁盘分区信息
                    // 我们的需求不会使用到，保持不变即可
                    // reference logical drive to partition
                    VARIANT vtProp1;
                    hres = pclsObj1->Get(_bstr_t(L"DeviceID"), 0, &vtProp1, 0, 0);
                    //printf("DeviceID:%ls\n",vtProp1.bstrVal);
                    wstring wstrQuery = L"Associators of {Win32_DiskPartition.DeviceID='";
                    wstrQuery += vtProp1.bstrVal;
                    wstrQuery += L"'} where AssocClass=Win32_LogicalDiskToPartition";
                    IEnumWbemClassObject* pEnumerator2 = NULL;
                    hres = pSvc->ExecQuery(
                               bstr_t("WQL"),
                               bstr_t(wstrQuery.c_str()),
                               WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                               NULL,
                               &pEnumerator2);

                    if (FAILED(hres))
                    {
                        cout << "Query for processes failed. "
                             << "Error code = 0x"
                             << hex << hres << endl;
                        pSvc->Release();
                        pLoc->Release();
                        CoUninitialize();
                        return FALSE;               // Program has failed.
                    }
                    else
                    {
                        // get driveletter
                        IWbemClassObject* pclsObj2;
                        ULONG uReturn2 = 0;

                        while (pEnumerator2)
                        {
                            hres = pEnumerator2->Next(WBEM_INFINITE, 1,
                                                      &pclsObj2, &uReturn2);

                            if (0 == uReturn2) break;

                            // 获取的是Win32_LogicalDisk的信息
                            // 例如：磁盘C,磁盘D都是同一硬盘的逻辑磁盘
                            // 可以在单独获取磁盘C、磁盘D的剩余空间，总空间信息
                            // 下面vtProp.bstrVal为上面获取的硬盘的型号
                            // vtProp2.bstrVal为磁盘的盘符C或D
                            // 分别获取磁盘的总大小和剩余大小。注意事项和硬盘部分描述一样
                            VARIANT vtProp2;
                            hres = pclsObj2->Get(_bstr_t(L"DeviceID"), 0, &vtProp2, 0, 0);

                            // print result
                            if (!judgeExistDisk(vtProp2.bstrVal))
                            {
                                printf("硬盘型号:%ls\t磁盘盘符:%ls ", vtProp.bstrVal, vtProp2.bstrVal);
                                //获取盘符剩余
                                PULARGE_INTEGER  pDiskFree;
                                PULARGE_INTEGER  pDiskTotal;
                                pDiskFree = new ULARGE_INTEGER;
                                pDiskTotal = new ULARGE_INTEGER;
                                GetDiskFreeSpaceExW(vtProp2.bstrVal, pDiskFree, pDiskTotal, NULL);
                                int64_t oneTotal = (ULONG)((pDiskTotal->QuadPart) / (1024 * 1024 * 1024));
                                int64_t oneFree = (ULONG)((pDiskFree->QuadPart) / (1024 * 1024 * 1024));
                                cout <<
                                     " 总大小:" << oneTotal << 'G' << " 剩余大小:" << oneFree << 'G' << endl;
                                // clear pDisknum
                                delete (pDiskFree);
                                pDiskFree = NULL;
                                delete (pDiskTotal);
                                pDiskTotal = NULL;
                            }

                            VariantClear(&vtProp2);
                        }

                        pclsObj1->Release();
                    }

                    VariantClear(&vtProp1);
                    pEnumerator2->Release();
                }

                pclsObj->Release();
            }

            VariantClear(&vtProp);
            pEnumerator1->Release();
        }
    }

    pEnumerator->Release();
    return TRUE;
}






BOOL wmi_close()
{
    // Cleanup
    // ========
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return 0;   // Program successfully completed.
}