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

// Registers: 0=A, 1=B, 2=C, 3=D, 4=E, 5=F
precz g_Regs[6];
const char* REG_NAMES[] = { "A", "B", "C", "D", "E", "F" };

// Colors
HBRUSH hBrushBtn;

// --- UI Elements ---
HWND hOutput; // Top Log

// Input Area
HWND hEditInput; // Large Input Box
HWND hComboInputReg; // "A", "B"...
HWND hBtnSetReg; // "-> Set"
HWND hBtnGetReg; // "<- Get"

// Operations Area
HWND hRadioGroup; // Implicit group
// 1. Basic
HWND hRadioBasic;
HWND hC_Basic_Res, hC_Basic_Op1, hC_Basic_Op2; 
HWND hComboBasicOp; // +, -, *, /, % (Moved to a dynamic combo between Op1/Op2)

// 2. Pow
HWND hRadioPow;
HWND hC_Pow_Res, hC_Pow_Base;
HWND hEdit_Pow_Exp; // Integer exponent

// 3. Sqrt
HWND hRadioSqrt;
HWND hC_Sqrt_Res, hC_Sqrt_Op;

// 4. Factor
HWND hRadioFactor;
HWND hC_Factor_Op;

// 5. GCD
HWND hRadioGCD;
HWND hC_GCD_Res, hC_GCD_Op1, hC_GCD_Op2;

HWND hBtnCalc;

// IDs
#define ID_OUTPUT        100
#define ID_EDIT_INPUT    101
#define ID_COMBO_IN_REG  102
#define ID_BTN_SET       103
#define ID_BTN_GET       104

#define ID_RADIO_BASIC   200
#define ID_RADIO_POW     201
#define ID_RADIO_SQRT    202
#define ID_RADIO_FACTOR  203
#define ID_RADIO_GCD     204

#define ID_BTN_CALC      300

// ======================= HELPERS ======================= 
std::string GetText(HWND h) {
    int len = GetWindowTextLength(h);
    if (len <= 0) return "";
    std::vector<char> buf(len + 1);
    GetWindowText(h, buf.data(), len + 1);
    return std::string(buf.data());
}

void SetText(HWND h, const std::string& s) {
    SetWindowText(h, s.c_str());
}

void AppendLine(HWND h, const std::string& s) {
    int len = GetWindowTextLength(h);
    SendMessage(h, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    std::string fin = s + "\r\n";
    SendMessage(h, EM_REPLACESEL, 0, (LPARAM)fin.c_str());
}

void PopulateRegCombo(HWND h) {
    for (int i=0; i<6; ++i) SendMessage(h, CB_ADDSTRING, 0, (LPARAM)REG_NAMES[i]);
    SendMessage(h, CB_SETCURSEL, 0, 0);
}

int GetComboSel(HWND h) {
    return (int)SendMessage(h, CB_GETCURSEL, 0, 0); 
}

// ======================= LOGIC ======================= 
void DoSetRegister() {
    int idx = GetComboSel(hComboInputReg);
    if (idx < 0 || idx > 5) return;
    
    std::string s = GetText(hEditInput);
    if (s.empty()) s = "0";
    
    try {
        g_Regs[idx] = precz(s);
        std::stringstream ss;
        ss << "Assigned Input to Register " << REG_NAMES[idx];
        AppendLine(hOutput, ss.str());
    } catch (...) {
        AppendLine(hOutput, "Error: Invalid Number Format");
    }
}

void DoGetRegister() {
    int idx = GetComboSel(hComboInputReg);
    if (idx < 0 || idx > 5) return;
    SetText(hEditInput, g_Regs[idx].to_string());
}

void DoCalculate() {
    std::stringstream log;
    try {
        if (SendMessage(hRadioBasic, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            int rRes = GetComboSel(hC_Basic_Res);
            int rOp1 = GetComboSel(hC_Basic_Op1);
            int rOp2 = GetComboSel(hC_Basic_Op2);
            int opType = GetComboSel(hComboBasicOp); // 0=+, 1=-, 2=*, 3=/, 4=%
            
            precz& A = g_Regs[rOp1];
            precz& B = g_Regs[rOp2];
            precz res;
            
            std::string opSym;
            switch(opType) {
                case 0: res = A + B; opSym = "+"; break;
                case 1: res = A - B; opSym = "-"; break;
                case 2: res = A * B; opSym = "*"; break;
                case 3: 
                    if (B.is_zero()) { log << "Error: Div by Check"; goto done; }
                    res = A / B; opSym = "/"; 
                    break;
                case 4: 
                    if (B.is_zero()) { log << "Error: Div by Check"; goto done; }
                    res = A % B; opSym = "%"; 
                    break;
                default: res = A + B; opSym = "+"; break;
            }
            
            g_Regs[rRes] = res;
            log << REG_NAMES[rRes] << " = " << REG_NAMES[rOp1] << " " << opSym << " " << REG_NAMES[rOp2];
            log << "\r\nResult (" << REG_NAMES[rRes] << ") = " << res;
        }
        else if (SendMessage(hRadioPow, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            int rRes = GetComboSel(hC_Pow_Res);
            int rBase = GetComboSel(hC_Pow_Base);
            std::string sExp = GetText(hEdit_Pow_Exp);
            
            precz base = g_Regs[rBase];
            long long exp = 0;
            try { exp = std::stoll(sExp); } catch(...) {}
            
            // Safety
            if (exp > 10000) { log << "Error: Exp > 10000 too slow for demo"; goto done; }
            if (exp < 0) { log << "Error: Negative Exp"; goto done; }
            
            precz res(1);
            precz b = base;
            long long e = exp;
            while(e > 0) {
                if(e&1) res = res*b;
                b = b*b;
                e>>=1;
            }
            
            g_Regs[rRes] = res;
            log << REG_NAMES[rRes] << " = " << REG_NAMES[rBase] << " ^ " << exp;
            log << "\r\nResult = " << res;
        }
        else if (SendMessage(hRadioSqrt, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            int rRes = GetComboSel(hC_Sqrt_Res);
            int rOp = GetComboSel(hC_Sqrt_Op);
            
            precz val = g_Regs[rOp];
            if (val < 0) { log << "Error: Negative Sqrt"; goto done; }
            
            // Newton for Sqrt(val)
            precz k = val;
            if (!val.is_zero()) {
               for(int i=0;i<100;++i) {
                   precz next = (k + val/k) >> 1;
                   if (next >= k) break;
                   k = next;
               }
            } else { k = 0; }
            
            g_Regs[rRes] = k;
            log << REG_NAMES[rRes] << " = Sqrt(" << REG_NAMES[rOp] << ")";
            log << "\r\nResult = " << k;
        }
        else if (SendMessage(hRadioFactor, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            int rOp = GetComboSel(hC_Factor_Op);
            precz val = g_Regs[rOp];
            
            log << "Factoring " << REG_NAMES[rOp] << " (" << val << ")...\r\n";
            prec_factor::factor_result_t factors;
            prec_factor::factorex(val, factors);
            
            log << val << " = ";
            for (size_t i = 0; i < factors.size(); ++i) {
                if (i > 0) log << " * ";
                log << factors[i].prime;
                if (factors[i].count > 1) log << "^" << factors[i].count;
            }
        }
        else if (SendMessage(hRadioGCD, BM_GETCHECK, 0, 0) == BST_CHECKED) {
             int rRes = GetComboSel(hC_GCD_Res);
             int rOp1 = GetComboSel(hC_GCD_Op1);
             int rOp2 = GetComboSel(hC_GCD_Op2);
             
             precz res = prec_factor::gcd(g_Regs[rOp1], g_Regs[rOp2]);
             g_Regs[rRes] = res;
             
             log << REG_NAMES[rRes] << " = GCD(" << REG_NAMES[rOp1] << ", " << REG_NAMES[rOp2] << ")";
             log << "\r\nResult = " << res;
        }

    } catch (...) {
        log << "Exception in Calculation.";
    }

done:
    AppendLine(hOutput, log.str());
    AppendLine(hOutput, "--------------------------------------------------");
}


// ======================= WNDPROC ======================= 
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
    {
        HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                                DEFAULT_PITCH | FF_DONTCARE, "Consolas");

        // --- TOP: Output ---
        hOutput = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "Ready.\r\n", 
            WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY,
            10, 10, 760, 200, hwnd, (HMENU)ID_OUTPUT, hInst, NULL);
        SendMessage(hOutput, WM_SETFONT, (WPARAM)hFont, TRUE);

        // --- LEFT: Input ---
        int inX = 10, inY = 220;
        CreateWindow("STATIC", "Input Value:", WS_CHILD|WS_VISIBLE, inX, inY, 300, 20, hwnd, NULL, hInst, NULL);
        
        hEditInput = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "123456789", 
            WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL,
            inX, inY+25, 300, 150, hwnd, (HMENU)ID_EDIT_INPUT, hInst, NULL);
        SendMessage(hEditInput, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Input Controls
        int ctrlY = inY + 185;
        CreateWindow("STATIC", "Register:", WS_CHILD|WS_VISIBLE, inX, ctrlY+4, 60, 20, hwnd, NULL, hInst, NULL);
        hComboInputReg = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,
            inX+60, ctrlY, 50, 200, hwnd, (HMENU)ID_COMBO_IN_REG, hInst, NULL);
        PopulateRegCombo(hComboInputReg); // A..F

        hBtnSetReg = CreateWindow("BUTTON", "Set ->", WS_CHILD|WS_VISIBLE, inX+120, ctrlY, 80, 25, hwnd, (HMENU)ID_BTN_SET, hInst, NULL);
        hBtnGetReg = CreateWindow("BUTTON", "<- Get", WS_CHILD|WS_VISIBLE, inX+210, ctrlY, 80, 25, hwnd, (HMENU)ID_BTN_GET, hInst, NULL);


        // --- RIGHT: Operations ---
        int opX = 330, opY = 220;
        CreateWindow("BUTTON", "Operations", WS_CHILD|WS_VISIBLE|BS_GROUPBOX, opX, opY, 440, 250, hwnd, NULL, hInst, NULL);
        
        // Shared Alignments
        int xRadio = opX + 15;
        int xRes   = opX + 40;  // Target Register Combo
        int xEq    = opX + 90;  // "=" sign
        int xStart = opX + 110; // Start of RHS

        // 1. Basic: [Res] = [Op1] [Op] [Op2]
        int row1 = opY + 25;
        hRadioBasic = CreateWindow("BUTTON", "", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON|WS_GROUP, 
            xRadio, row1+3, 15, 15, hwnd, (HMENU)ID_RADIO_BASIC, hInst, NULL);
        SendMessage(hRadioBasic, BM_SETCHECK, BST_CHECKED, 0);

        hC_Basic_Res = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xRes, row1, 45, 200, hwnd, NULL, hInst, NULL);
        PopulateRegCombo(hC_Basic_Res); // Def A

        CreateWindow("STATIC", "=", WS_CHILD|WS_VISIBLE, xEq, row1+3, 15, 20, hwnd, NULL, hInst, NULL);

        hC_Basic_Op1 = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xStart, row1, 45, 200, hwnd, NULL, hInst, NULL);
        PopulateRegCombo(hC_Basic_Op1); 
        SendMessage(hC_Basic_Op1, CB_SETCURSEL, 1, 0); // B

        hComboBasicOp = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xStart+50, row1, 45, 200, hwnd, NULL, hInst, NULL);
        const char* ops[] = {"+", "-", "*", "/", "%"};
        for(auto o:ops) SendMessage(hComboBasicOp, CB_ADDSTRING, 0, (LPARAM)o);
        SendMessage(hComboBasicOp, CB_SETCURSEL, 2, 0); // *

        hC_Basic_Op2 = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xStart+100, row1, 45, 200, hwnd, NULL, hInst, NULL);
        PopulateRegCombo(hC_Basic_Op2);
        SendMessage(hC_Basic_Op2, CB_SETCURSEL, 2, 0); // C


        // 2. Pow: [Res] = Pow( [Base] , [Exp] )
        int row2 = opY + 60;
        hRadioPow = CreateWindow("BUTTON", "", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON, 
            xRadio, row2+3, 15, 15, hwnd, (HMENU)ID_RADIO_POW, hInst, NULL);
        
        hC_Pow_Res = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xRes, row2, 45, 200, hwnd, NULL, hInst, NULL);
        PopulateRegCombo(hC_Pow_Res); 
        
        CreateWindow("STATIC", "= Pow(", WS_CHILD|WS_VISIBLE, xEq, row2+3, 50, 20, hwnd, NULL, hInst, NULL);
        
        hC_Pow_Base = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xEq+55, row2, 45, 200, hwnd, NULL, hInst, NULL);
        PopulateRegCombo(hC_Pow_Base); 
        SendMessage(hC_Pow_Base, CB_SETCURSEL, 3, 0); // D

        CreateWindow("STATIC", ",", WS_CHILD|WS_VISIBLE, xEq+105, row2+3, 10, 20, hwnd, NULL, hInst, NULL);
        
        hEdit_Pow_Exp = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "1000", WS_CHILD|WS_VISIBLE, xEq+115, row2, 60, 22, hwnd, NULL, hInst, NULL);
        
        CreateWindow("STATIC", ")", WS_CHILD|WS_VISIBLE, xEq+180, row2+3, 10, 20, hwnd, NULL, hInst, NULL);


        // 3. Sqrt: [Res] = Sqrt( [Op] )
        int row3 = opY + 95;
        hRadioSqrt = CreateWindow("BUTTON", "", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON, 
            xRadio, row3+3, 15, 15, hwnd, (HMENU)ID_RADIO_SQRT, hInst, NULL);
        
        hC_Sqrt_Res = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xRes, row3, 45, 200, hwnd, NULL, hInst, NULL);
        PopulateRegCombo(hC_Sqrt_Res); 

        CreateWindow("STATIC", "= Sqrt(", WS_CHILD|WS_VISIBLE, xEq, row3+3, 50, 20, hwnd, NULL, hInst, NULL);
        
        hC_Sqrt_Op = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xEq+55, row3, 45, 200, hwnd, NULL, hInst, NULL);
        PopulateRegCombo(hC_Sqrt_Op);
        SendMessage(hC_Sqrt_Op, CB_SETCURSEL, 4, 0); // E

        CreateWindow("STATIC", ")", WS_CHILD|WS_VISIBLE, xEq+105, row3+3, 10, 20, hwnd, NULL, hInst, NULL);


        // 4. GCD: [Res] = GCD( [Op1], [Op2] )
        int row4 = opY + 130;
        hRadioGCD = CreateWindow("BUTTON", "", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON, 
            xRadio, row4+3, 15, 15, hwnd, (HMENU)ID_RADIO_GCD, hInst, NULL);
        
        hC_GCD_Res = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xRes, row4, 45, 200, hwnd, NULL, hInst, NULL);
        PopulateRegCombo(hC_GCD_Res); 

        CreateWindow("STATIC", "= GCD(", WS_CHILD|WS_VISIBLE, xEq, row4+3, 50, 20, hwnd, NULL, hInst, NULL);

        hC_GCD_Op1 = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xEq+55, row4, 45, 200, hwnd, NULL, hInst, NULL);
        PopulateRegCombo(hC_GCD_Op1);

        CreateWindow("STATIC", ",", WS_CHILD|WS_VISIBLE, xEq+105, row4+3, 10, 20, hwnd, NULL, hInst, NULL);

        hC_GCD_Op2 = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xEq+120, row4, 45, 200, hwnd, NULL, hInst, NULL);
        PopulateRegCombo(hC_GCD_Op2);
        
        CreateWindow("STATIC", ")", WS_CHILD|WS_VISIBLE, xEq+170, row4+3, 10, 20, hwnd, NULL, hInst, NULL);


        // 5. Factor: Factor( [Op] ) -> Output
        int row5 = opY + 165;
        hRadioFactor = CreateWindow("BUTTON", "", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON, 
            xRadio, row5+3, 15, 15, hwnd, (HMENU)ID_RADIO_FACTOR, hInst, NULL);
        
        CreateWindow("STATIC", "Factorize(", WS_CHILD|WS_VISIBLE, xRes, row5+3, 70, 20, hwnd, NULL, hInst, NULL);
        hC_Factor_Op = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, xRes+75, row5, 45, 200, hwnd, NULL, hInst, NULL);
        PopulateRegCombo(hC_Factor_Op);
        CreateWindow("STATIC", ") -> Output", WS_CHILD|WS_VISIBLE, xRes+125, row5+3, 80, 20, hwnd, NULL, hInst, NULL);


        // Calculate
        hBtnCalc = CreateWindow("BUTTON", "Calculate", WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON, 
            opX + 250, opY + 200, 150, 40, hwnd, (HMENU)ID_BTN_CALC, hInst, NULL);
        SendMessage(hBtnCalc, WM_SETFONT, (WPARAM)hFont, TRUE);

    } return 0;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
            case ID_BTN_SET: 
                DoSetRegister(); 
                break;
            case ID_BTN_GET: 
                DoGetRegister(); 
                break;
            case ID_BTN_CALC: 
                SetCursor(LoadCursor(NULL, IDC_WAIT));
                DoCalculate();
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                break;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "MiniCalcV3";
    RegisterClassEx(&wc);

    hMain = CreateWindowEx(0, "MiniCalcV3", "MiniCalc", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 520, 
        NULL, NULL, hInstance, NULL);

    ShowWindow(hMain, nCmdShow);
    UpdateWindow(hMain);

    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
