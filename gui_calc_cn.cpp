#define _CRT_SECURE_NO_WARNINGS
#undef UNICODE
#undef _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <algorithm>

#include "prec.hpp"
#include "prec_factor.hpp"

// ======================= GLOBALS ======================= 
HINSTANCE hInst;
HWND hMain;

precz g_Regs[6];
const char* REG_NAMES[] = { "A", "B", "C", "D", "E", "F" };
const wchar_t* REG_NAMES_SHORT[] = { L"A", L"B", L"C", L"D", L"E", L"F" };

inline const wchar_t* RegLabel(int idx) {
    if (idx < 0 || idx >= 6) return L"";
    return REG_NAMES_SHORT[idx];
}

HWND hOutput;
HWND hEditInput;
HWND hComboInputReg;
HWND hBtnSetReg;
HWND hBtnGetReg;
HWND hRadioGroup;
HWND hRadioBasic;
HWND hC_Basic_Res, hC_Basic_Op1, hC_Basic_Op2;
HWND hComboBasicOp;
HWND hRadioPow;
HWND hC_Pow_Res, hC_Pow_Base;
HWND hEdit_Pow_Exp;
HWND hRadioSqrt;
HWND hC_Sqrt_Res, hC_Sqrt_Op;
HWND hRadioGCD;
HWND hC_GCD_Res, hC_GCD_Op1, hC_GCD_Op2;
HWND hBtnCalc;

#define ID_OUTPUT        100
#define ID_EDIT_INPUT    101
#define ID_COMBO_IN_REG  102
#define ID_BTN_SET       103
#define ID_BTN_GET       104
#define ID_RADIO_BASIC   200
#define ID_RADIO_POW     201
#define ID_RADIO_SQRT    202
// #define ID_RADIO_FACTOR  203
#define ID_RADIO_GCD     204
#define ID_BTN_CALC      300

std::wstring GetTextW(HWND h) {
    int len = GetWindowTextLengthW(h);
    if (len <= 0) return L"";
    std::vector<wchar_t> buf(len + 1);
    GetWindowTextW(h, buf.data(), len + 1);
    return std::wstring(buf.data());
}
void SetTextW(HWND h, const std::wstring& s) {
    SetWindowTextW(h, s.c_str());
}
void AppendLineW(HWND h, const std::wstring& s) {
    int len = GetWindowTextLengthW(h);
    SendMessageW(h, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    std::wstring fin = s + L"\r\n";
    SendMessageW(h, EM_REPLACESEL, 0, (LPARAM)fin.c_str());
}
void AppendLinesW(HWND h, const std::wstring& text) {
    size_t start = 0;
    while (start < text.size()) {
        size_t end = text.find(L'\n', start);
        size_t len = (end == std::wstring::npos) ? text.size() - start : end - start;
        std::wstring line = text.substr(start, len);
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        AppendLineW(h, line);
        if (end == std::wstring::npos) break;
        start = end + 1;
    }
    if (text.empty()) AppendLineW(h, L"");
}
void PopulateRegComboW(HWND h) {
    for (int i=0; i<6; ++i) SendMessageW(h, CB_ADDSTRING, 0, (LPARAM)REG_NAMES_SHORT[i]);
    SendMessageW(h, CB_SETCURSEL, 0, 0);
}
int GetComboSel(HWND h) {
    return (int)SendMessageW(h, CB_GETCURSEL, 0, 0); 
}
void DoSetRegisterW() {
    int idx = GetComboSel(hComboInputReg);
    if (idx < 0 || idx > 5) return;
    std::wstring s = GetTextW(hEditInput);
    if (s.empty()) s = L"0";
    try {
        std::string s_utf8(s.begin(), s.end());
        g_Regs[idx] = precz(s_utf8);
        std::wstringstream ss;
        ss << L"已将输入值赋给" << RegLabel(idx);
        AppendLineW(hOutput, ss.str());
    } catch (...) {
        AppendLineW(hOutput, L"错误：无效数字格式");
    }
}
void DoGetRegisterW() {
    int idx = GetComboSel(hComboInputReg);
    if (idx < 0 || idx > 5) return;
    std::string val = g_Regs[idx].to_string();
    std::wstring wval(val.begin(), val.end());
    SetTextW(hEditInput, wval);
}
void DoCalculateW() {
    std::wstringstream log;
    try {
        if (SendMessageW(hRadioBasic, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            int rRes = GetComboSel(hC_Basic_Res);
            int rOp1 = GetComboSel(hC_Basic_Op1);
            int rOp2 = GetComboSel(hC_Basic_Op2);
            int opType = GetComboSel(hComboBasicOp);
            precz& A = g_Regs[rOp1];
            precz& B = g_Regs[rOp2];
            precz res;
            std::wstring opSym;
            switch(opType) {
                case 0: res = A + B; opSym = L"+"; break;
                case 1: res = A - B; opSym = L"-"; break;
                case 2: res = A * B; opSym = L"×"; break;
                case 3: if (B.is_zero()) { log << L"错误：除数为零"; goto done; } res = A / B; opSym = L"÷"; break;
                case 4: if (B.is_zero()) { log << L"错误：除数为零"; goto done; } res = A % B; opSym = L"模"; break;
                default: res = A + B; opSym = L"+"; break;
            }
            g_Regs[rRes] = res;
            log << RegLabel(rRes) << L" = " << RegLabel(rOp1) << L" " << opSym << L" " << RegLabel(rOp2);
            std::string s_res = res.to_string();
            log << L"\r\n结果(" << RegLabel(rRes) << L") = " << std::wstring(s_res.begin(), s_res.end());
        }
        else if (SendMessageW(hRadioPow, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            int rRes = GetComboSel(hC_Pow_Res);
            int rBase = GetComboSel(hC_Pow_Base);
            std::wstring sExp = GetTextW(hEdit_Pow_Exp);
            precz base = g_Regs[rBase];
            long long exp = 0;
            try { exp = std::stoll(sExp); } catch(...) {}
            if (exp > 10000) { log << L"错误：指数过大"; goto done; }
            if (exp < 0) { log << L"错误：负指数"; goto done; }
            precz res(1);
            precz b = base;
            long long e = exp;
            while(e > 0) {
                if(e&1) res = res*b;
                b = b*b;
                e>>=1;
            }
            g_Regs[rRes] = res;
            log << RegLabel(rRes) << L" = " << RegLabel(rBase) << L" ^ " << exp;
            log << L"\r\n计算完成，正在转换结果...";
            std::string s_res = res.to_string();
            log << L"\r\n结果 = " << std::wstring(s_res.begin(), s_res.end());
        }
        else if (SendMessageW(hRadioSqrt, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            int rRes = GetComboSel(hC_Sqrt_Res);
            int rOp = GetComboSel(hC_Sqrt_Op);
            precz val = g_Regs[rOp];
            if (val < 0) { log << L"错误：负数不能开方"; goto done; }
            precz k = val;
            if (!val.is_zero()) {
               for(int i=0;i<100;++i) {
                   precz next = (k + val/k) >> 1;
                   if (next >= k) break;
                   k = next;
               }
            } else { k = 0; }
            g_Regs[rRes] = k;
            log << RegLabel(rRes) << L" = 开方(" << RegLabel(rOp) << L")";
            std::string s_k = k.to_string();
            log << L"\r\n结果 = " << std::wstring(s_k.begin(), s_k.end());
        }
        else if (SendMessageW(hRadioGCD, BM_GETCHECK, 0, 0) == BST_CHECKED) {
             int rRes = GetComboSel(hC_GCD_Res);
             int rOp1 = GetComboSel(hC_GCD_Op1);
             int rOp2 = GetComboSel(hC_GCD_Op2);
             precz res = prec_factor::gcd(g_Regs[rOp1], g_Regs[rOp2]);
             g_Regs[rRes] = res;
             log << RegLabel(rRes) << L" = 最大公约数(" << RegLabel(rOp1) << L", " << RegLabel(rOp2) << L")";
             std::string s_res = res.to_string();
             log << L"\r\n结果 = " << std::wstring(s_res.begin(), s_res.end());
        }
    } catch (const std::exception& e) {
        std::string what = e.what();
        std::wstring wwhat(what.begin(), what.end());
        log << L"计算时发生异常: " << wwhat;
    } catch (...) {
        log << L"计算时发生异常: 未知错误";
    }
done:
    AppendLinesW(hOutput, log.str());
    AppendLineW(hOutput, L"--------------------------------------------------");
}
LRESULT CALLBACK WndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
    {
        HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                DEFAULT_PITCH | FF_DONTCARE, L"微软雅黑");
        hOutput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"准备就绪。\r\n", 
            WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY,
            10, 10, 760, 200, hwnd, (HMENU)ID_OUTPUT, hInst, NULL);
        SendMessageW(hOutput, WM_SETFONT, (WPARAM)hFont, TRUE);
        int inX = 10, inY = 220;
        CreateWindowW(L"STATIC", L"输入数值：", WS_CHILD|WS_VISIBLE, inX, inY, 300, 20, hwnd, NULL, hInst, NULL);
        hEditInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"123456789", 
            WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL,
            inX, inY+25, 300, 150, hwnd, (HMENU)ID_EDIT_INPUT, hInst, NULL);
        SendMessageW(hEditInput, WM_SETFONT, (WPARAM)hFont, TRUE);
        int ctrlY = inY + 185;
        CreateWindowW(L"STATIC", L"寄存器", WS_CHILD|WS_VISIBLE, inX, ctrlY+2, 60, 20, hwnd, NULL, hInst, NULL);
        hComboInputReg = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,
            inX+65, ctrlY, 90, 200, hwnd, (HMENU)ID_COMBO_IN_REG, hInst, NULL);
        PopulateRegComboW(hComboInputReg);
        hBtnSetReg = CreateWindowW(L"BUTTON", L"设置 ->", WS_CHILD|WS_VISIBLE, inX+160, ctrlY, 70, 25, hwnd, (HMENU)ID_BTN_SET, hInst, NULL);
        hBtnGetReg = CreateWindowW(L"BUTTON", L"<- 读取", WS_CHILD|WS_VISIBLE, inX+235, ctrlY, 70, 25, hwnd, (HMENU)ID_BTN_GET, hInst, NULL);
        int opX = 400, opY = 220;
        CreateWindowW(L"BUTTON", L"运算区", WS_CHILD|WS_VISIBLE|BS_GROUPBOX, opX, opY, 460, 250, hwnd, NULL, hInst, NULL);
        int xRadio = opX + 15;
        int xRes   = opX + 40;
        int shift  = 35;
        int xEq    = opX + 90 + shift;
        int xStart = opX + 110 + shift;
        int row1 = opY + 25;
        hRadioBasic = CreateWindowW(L"BUTTON", L"", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON|WS_GROUP, 
            xRadio, row1+3, 15, 15, hwnd, (HMENU)ID_RADIO_BASIC, hInst, NULL);
        SendMessageW(hRadioBasic, BM_SETCHECK, BST_CHECKED, 0);
        hC_Basic_Res = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xRes, row1, 70, 200, hwnd, NULL, hInst, NULL);
        PopulateRegComboW(hC_Basic_Res);
        CreateWindowW(L"STATIC", L"=", WS_CHILD|WS_VISIBLE, xEq, row1+3, 15, 20, hwnd, NULL, hInst, NULL);
        hC_Basic_Op1 = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xStart, row1, 70, 200, hwnd, NULL, hInst, NULL);
        PopulateRegComboW(hC_Basic_Op1);
        SendMessageW(hC_Basic_Op1, CB_SETCURSEL, 1, 0);
        hComboBasicOp = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xStart+75, row1, 45, 200, hwnd, NULL, hInst, NULL);
        const wchar_t* ops[] = {L"+", L"-", L"×", L"÷", L"模"};
        for(auto o:ops) SendMessageW(hComboBasicOp, CB_ADDSTRING, 0, (LPARAM)o);
        SendMessageW(hComboBasicOp, CB_SETCURSEL, 2, 0);
        hC_Basic_Op2 = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xStart+130, row1, 70, 200, hwnd, NULL, hInst, NULL);
        PopulateRegComboW(hC_Basic_Op2);
        SendMessageW(hC_Basic_Op2, CB_SETCURSEL, 2, 0);
        int row2 = opY + 60;
        hRadioPow = CreateWindowW(L"BUTTON", L"", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON, 
            xRadio, row2+3, 15, 15, hwnd, (HMENU)ID_RADIO_POW, hInst, NULL);
        hC_Pow_Res = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xRes, row2, 70, 200, hwnd, NULL, hInst, NULL);
        PopulateRegComboW(hC_Pow_Res);
        CreateWindowW(L"STATIC", L"= 幂(", WS_CHILD|WS_VISIBLE, xEq, row2+3, 50, 20, hwnd, NULL, hInst, NULL);
        hC_Pow_Base = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xEq+55, row2, 70, 200, hwnd, NULL, hInst, NULL);
        PopulateRegComboW(hC_Pow_Base);
        SendMessageW(hC_Pow_Base, CB_SETCURSEL, 3, 0);
        CreateWindowW(L"STATIC", L",", WS_CHILD|WS_VISIBLE, xEq+130, row2+3, 10, 20, hwnd, NULL, hInst, NULL);
        hEdit_Pow_Exp = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1000", WS_CHILD|WS_VISIBLE, xEq+145, row2, 60, 22, hwnd, NULL, hInst, NULL);
        CreateWindowW(L"STATIC", L")", WS_CHILD|WS_VISIBLE, xEq+210, row2+3, 10, 20, hwnd, NULL, hInst, NULL);
        int row3 = opY + 95;
        hRadioSqrt = CreateWindowW(L"BUTTON", L"", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON, 
            xRadio, row3+3, 15, 15, hwnd, (HMENU)ID_RADIO_SQRT, hInst, NULL);
        hC_Sqrt_Res = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xRes, row3, 70, 200, hwnd, NULL, hInst, NULL);
        PopulateRegComboW(hC_Sqrt_Res);
        CreateWindowW(L"STATIC", L"= 开方(", WS_CHILD|WS_VISIBLE, xEq, row3+3, 50, 20, hwnd, NULL, hInst, NULL);
        hC_Sqrt_Op = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xEq+55, row3, 70, 200, hwnd, NULL, hInst, NULL);
        PopulateRegComboW(hC_Sqrt_Op);
        SendMessageW(hC_Sqrt_Op, CB_SETCURSEL, 4, 0);
        CreateWindowW(L"STATIC", L")", WS_CHILD|WS_VISIBLE, xEq+130, row3+3, 10, 20, hwnd, NULL, hInst, NULL);
        int row4 = opY + 130;
        hRadioGCD = CreateWindowW(L"BUTTON", L"", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON, 
            xRadio, row4+3, 15, 15, hwnd, (HMENU)ID_RADIO_GCD, hInst, NULL);
        hC_GCD_Res = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xRes, row4, 70, 200, hwnd, NULL, hInst, NULL);
        PopulateRegComboW(hC_GCD_Res);
        int gcdLabelX = xRes + 40 + shift;
        CreateWindowW(L"STATIC", L"= 最大公约数(", WS_CHILD|WS_VISIBLE, gcdLabelX, row4+3, 140, 20, hwnd, NULL, hInst, NULL);
        int gcdOp1X = gcdLabelX + 150;
        hC_GCD_Op1 = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, gcdOp1X, row4, 75, 200, hwnd, NULL, hInst, NULL);
        PopulateRegComboW(hC_GCD_Op1);
        CreateWindowW(L"STATIC", L",", WS_CHILD|WS_VISIBLE, gcdOp1X + 80, row4+3, 10, 20, hwnd, NULL, hInst, NULL);
        int gcdOp2X = gcdOp1X + 110;
        hC_GCD_Op2 = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, gcdOp2X, row4, 75, 200, hwnd, NULL, hInst, NULL);
        PopulateRegComboW(hC_GCD_Op2);
        CreateWindowW(L"STATIC", L")", WS_CHILD|WS_VISIBLE, gcdOp2X + 80, row4+3, 10, 20, hwnd, NULL, hInst, NULL);
        hBtnCalc = CreateWindowW(L"BUTTON", L"计算", WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON, 
            opX + 280, opY + 200, 150, 40, hwnd, (HMENU)ID_BTN_CALC, hInst, NULL);
        SendMessageW(hBtnCalc, WM_SETFONT, (WPARAM)hFont, TRUE);
    } return 0;
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
            case ID_BTN_SET: DoSetRegisterW(); break;
            case ID_BTN_GET: DoGetRegisterW(); break;
            case ID_BTN_CALC: SetCursor(LoadCursor(NULL, IDC_WAIT)); DoCalculateW(); SetCursor(LoadCursor(NULL, IDC_ARROW)); break;
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProcW;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(IDI_APPLICATION));
    wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_ARROW));
    HBRUSH hBgBrush = CreateSolidBrush(RGB(242, 242, 242));
    wc.hbrBackground = hBgBrush;
    wc.lpszClassName = L"MiniCalcCN";
    RegisterClassExW(&wc);
    hMain = CreateWindowExW(0, L"MiniCalcCN", L"MiniCalc V1.0", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 520, 
        NULL, NULL, hInstance, NULL);
    // Use a more visually appealing font, similar to MiniCalc
    HFONT hFont = CreateFontW(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
                              DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
    ShowWindow(hMain, nCmdShow);
    UpdateWindow(hMain);
    // Set font for all child controls after window creation
    EnumChildWindows(hMain, [](HWND hwnd, LPARAM lParam) -> BOOL {
        SendMessageW(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
        return TRUE;
    }, (LPARAM)hFont);
    MSG Msg;
    while (GetMessageW(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessageW(&Msg);
    }
    DeleteObject(hFont);
    DeleteObject(hBgBrush);
    return (int)Msg.wParam;
}
