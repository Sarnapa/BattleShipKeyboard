#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#define SHIP_SPEED 10
#define MISSILE_SPEED 10

// struktura do wspolrzednych statku
struct ShipRectangle
{
	int leftRect, topRect, width, height;
	
	ShipRectangle(int leftRect, int topRect, int width, int height)
	{
		this->leftRect = leftRect;
		this->topRect = topRect;
		this->width = width;
		this->height = height;
	}
};

// struktura pocisku
struct Missile
{
	bool active;
	int left, top, diameter;

	Missile(bool active, int left, int top, int diameter)
	{
		this->active = active;
		this->left = left;
		this->top = top;
		this->diameter = diameter;
	}
};

LPCWSTR windowClassName = _T("BattleShip");
LPCWSTR windowTitle = _T("BattleShip Keys");

const int SIZE_X = 640;
const int SIZE_Y = 400;
const int MIN_X = 20;
const int MAX_X = 580;
int MIN_Y, MAX_Y;

ShipRectangle shipRect(MIN_X, 150, 100, 50);
int firstScore = 0;
int secondScore = 0;
int activeMissiles = 0;
Missile missiles[4] = { Missile(false, 0, 0, 20), Missile(false, 0, 0, 20), Missile(false, 0, 0, 20), Missile(false, 0, 0, 20) };

MSG msg;
//uchwyt do kontekstu urzadzenia, uchwyt do bufora do podwojnego buforowania
HDC hdc, memDc;
// uchwyt do pedzli
HBRUSH hBlackBrush, hWhiteBrush;
//przetrzymywanie wspolrzednych naszego okna roboczego
RECT rectWindow;

//uchwyt do okna, kod komunikatu, dodatkowe parametry
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void draw();

// parametry: uchwyt aplikacji - numer jej wystapienia, uchwyt poprzedniego wystapienia, linia polecen z ktorej zostal uruchomiony nasz program (string),
// stan okna aplikacji
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// nie WNDCLASS, bo struktura WNDCLASSEX ma np. pole na mala ikonke
	WNDCLASSEX wc;

	//Rozmiar struktury w bajtach
	wc.cbSize = sizeof(WNDCLASSEX);
	//Style klasy (CS_HREDRAW - odrysowanie okna jesli ruch lub zmiana rozmiaru w szerokosci, VREDRAW - analogicznie dla wysokosci)
	wc.style = 0; //CS_HREDRAW | CS_VREDRAW;
	//Wskaznik do procedury obslugujacej okienko
	wc.lpfnWndProc = WndProc;
	//Dodatkowe bajty pamieci dla klasy
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	//Identyfikator aplikacji bedacej wlascicielem okna
	wc.hInstance = hInstance;
	//Duza Ikonka - na razie domyslna
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	//Kursor myszki - na razie domyslny (strzalka)
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	//Tlo okienka - domyslne szare tlo
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	//Nazwa do identyfikacji menu w zasobach - nie ma to null
	wc.lpszMenuName = NULL;
	//Nazwa klasy
	wc.lpszClassName = windowClassName;
	//Mala ikonka
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	//rejestracja klasy
	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, _T("Cannot register window class!"), NULL, MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}

	DWORD dwStyle = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX); // zeby nie bylo powiekszania,zmniejszania okna

	//parametry: nazwa klasy okna, tytul okna, styl okna, pozycja x2, rozmiar x2, uchwyt rodzica, uchwyt do menu, uchwyt do instancji aplikacji,
	//wskaznik do dodatkowych parametrow
	HWND hwnd = CreateWindow(windowClassName, windowTitle, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, SIZE_X, SIZE_Y,
							HWND_DESKTOP, NULL, hInstance, NULL);

	if (!hwnd)
	{
		MessageBox(NULL, _T("Cannot create window!"), NULL, MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}
	hdc = GetDC(hwnd);
	//tworzymy bufor na podwojne buforowanie
	memDc = CreateCompatibleDC(hdc);
	GetClientRect(hwnd, &rectWindow);
	HBITMAP hBitMap = CreateCompatibleBitmap(hdc, rectWindow.right, rectWindow.bottom);
	SelectObject(memDc, hBitMap);
	//tworzenie pedzli
	hBlackBrush = CreateSolidBrush(0x000000);
	hWhiteBrush = CreateSolidBrush(0xFFFFFF);
	//tworzymy timer - bedzie obslugiwany przez funkcje obslugi okna
	if (SetTimer(hwnd, 1, 20, NULL) == 0)
	{
		MessageBox(NULL, _T("Cannot create Timer!"), NULL, MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}
	//ustawienie trybu odwzorowania
	SetMapMode(hdc, MM_LOMETRIC);

	MIN_Y = rectWindow.top + 20;
	MAX_Y = rectWindow.bottom - 20;

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	// pobieranie komunikatow z kolejki
	// parametry: komunikat, uchwyt do okna, 2xfiltr komunikatow -> jesli 2x0 to bierzemy wszystkie komunikaty
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg); // przeslanie do funkcji obslugi
	}

	DeleteObject(hWhiteBrush);
	DeleteObject(hBlackBrush);
	DeleteObject(hBitMap);
	DeleteDC(memDc);

	return msg.wParam;
}

//uchwyt do okna, kod komunikatu, dodatkowe parametry
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps; //zawiera informacje o odswiezanym obszarze
	switch (msg)
	{
		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			draw();
			EndPaint(hwnd, &ps);
			break;
		case WM_KEYDOWN:
			switch ((int)wParam)
			{
				case VK_UP:
					if (shipRect.topRect > MIN_Y)
					{
						hdc = GetDC(hwnd);
						shipRect.topRect -= SHIP_SPEED;
						draw();
						ReleaseDC(hwnd, hdc);
					}
					break;
				case VK_DOWN:
					if (shipRect.topRect + shipRect.height < MAX_Y)
					{
						hdc = GetDC(hwnd);
						shipRect.topRect += SHIP_SPEED;
						draw();
						ReleaseDC(hwnd, hdc);
					}
					break;
				case VK_SPACE:
					if (activeMissiles < 2)
					{
						++activeMissiles;
						hdc = GetDC(hwnd);
						if (!missiles[0].active)
						{
							missiles[0].active = true;
							missiles[0].left = shipRect.leftRect + shipRect.width + 10;
							missiles[0].top = shipRect.topRect + 15;
						}
						else
						{
							missiles[1].active = true;
							missiles[1].left = shipRect.leftRect + shipRect.width + 10;
							missiles[1].top = shipRect.topRect + 15;
						}
						draw();
						ReleaseDC(hwnd, hdc);
					}
					break;
				case VK_ESCAPE:
					DestroyWindow(hwnd); // // generuje WM_DESTROY
					break;
			}
			break;
		case WM_TIMER:
			hdc = GetDC(hwnd);
			for (int i = 0; i < 4; ++i)
			{
				if (missiles[i].active)
				{
					int speed = (i <= 2) ? MISSILE_SPEED : -MISSILE_SPEED;
					missiles[i].left += MISSILE_SPEED;
					if (i <= 2 && missiles[i].left + missiles[i].diameter > MAX_X)
					{
						missiles[i].active = false;
						--activeMissiles;
					}
					else if (i > 2 && missiles[i].left < MIN_X)
					{

					}
				}
			}
			draw();
			ReleaseDC(hwnd, hdc);
			break;
		case WM_CLOSE:
			DestroyWindow(hwnd); // generuje WM_DESTROY
			break;
		case WM_DESTROY:
			KillTimer(hwnd, 1);
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam); // system obsluzy
	}
	return 0;
}

void draw()
{
	SelectObject(memDc, hBlackBrush);
	FillRect(memDc, &rectWindow, hWhiteBrush);
	Rectangle(memDc, shipRect.leftRect, shipRect.topRect, shipRect.leftRect + shipRect.width, shipRect.topRect + shipRect.height);
	for (int i = 0; i < 4; ++i)
		if (missiles[i].active)
			Ellipse(memDc, missiles[i].left, missiles[i].top, missiles[i].left + missiles[i].diameter, missiles[i].top + missiles[i].diameter);
	BitBlt(hdc, 0, 0, rectWindow.right, rectWindow.bottom, memDc, 0, 0, SRCCOPY);
}