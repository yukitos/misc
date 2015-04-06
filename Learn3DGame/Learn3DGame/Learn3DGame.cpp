// Learn3D.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Learn3DGame.h"

#define ROT_SPEED (XM_PI / 100.0f)
#define CORNER_NUM 50
#define R 2.0f

struct CUSTOMVERTEX {
    XMFLOAT4 v4Pos;
    XMFLOAT3 v3Normal;
};

struct CBNeverChanges
{
    XMMATRIX matView;
    XMMATRIX matWorld;
    XMFLOAT3 v3Light;
    float fAmbient;
    float fDirectional;
};

struct TEX_PICTURE
{
    ID3D11ShaderResourceView *pSRViewTexture;
    D3D11_TEXTURE2D_DESC tdDesc;
    int nWidth, nHeight;
};

UINT g_nClientWidth;
UINT g_nClienthHeight;

HWND g_hWnd;

ID3D11Device *g_pd3dDevice;
IDXGISwapChain *g_pSwapChain;
ID3D11DeviceContext *g_pImmediateContext;
ID3D11RasterizerState *g_pRS;
ID3D11RenderTargetView *g_pRTV;
ID3D11Texture2D *g_pDepthStencil;
ID3D11DepthStencilView *g_pDepthStencilView;
D3D_FEATURE_LEVEL g_FeatureLevel;

ID3D11Buffer *g_pVertexBuffer;
ID3D11Buffer *g_pIndexBuffer;
ID3D11BlendState *g_pbsAlphaBlend;
ID3D11VertexShader *g_pVertexShader;
ID3D11PixelShader *g_pPixelShader;
ID3D11InputLayout *g_pInputLayout;
ID3D11SamplerState *g_pSamplerState;

ID3D11Buffer *g_pCBNeverChanges;

CUSTOMVERTEX g_cvVertices[MAX_BUFFER_VERTEX];
int g_nVertexNum = 0;

WORD g_wIndices[MAX_BUFFER_INDEX];
int g_nIndexNum = 0;

TEX_PICTURE g_tTexture;

ID3D11ShaderResourceView *g_pNowTexture;

void ShowError(LPTSTR msg, LPTSTR title = _T("ERROR"), HWND hWnd = nullptr) {
    MessageBox(hWnd, msg, title, MB_ICONERROR | MB_OK);
}

HRESULT InitD3D(void) {
    HRESULT hr = S_OK;
    D3D_FEATURE_LEVEL FeatureLevelsRequested[6] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };
    UINT numLevelsRequested = ARRAYSIZE(FeatureLevelsRequested);
    D3D_FEATURE_LEVEL FeatureLevelsSupported;

    hr = D3D11CreateDevice(NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        0,
        FeatureLevelsRequested,
        numLevelsRequested,
        D3D11_SDK_VERSION,
        &g_pd3dDevice,
        &FeatureLevelsSupported,
        &g_pImmediateContext);
    if (FAILED(hr)) {
        ShowError(_T("Failed to create D3D11 device."));
        return hr;
    }

    IDXGIDevice *pDXGIDevice;
    hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice);
    if (FAILED(hr)) {
        ShowError(_T("Failed to query IDXGIDevice."));
        return hr;
    }
    IDXGIAdapter *pDXGIAdapter;
    hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&pDXGIAdapter);
    if (FAILED(hr)) {
        ShowError(_T("Failed to get IDXGIAdapter."));
        pDXGIDevice->Release();
        return hr;
    }
    IDXGIFactory *pIDXGIFactory;
    hr = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pIDXGIFactory);
    if (FAILED(hr)) {
        ShowError(_T("Failed to get IDXGIFactory."));
        pDXGIAdapter->Release();
        pDXGIDevice->Release();
        return hr;
    }

    {
        DXGI_SWAP_CHAIN_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.BufferCount = 1;
        desc.BufferDesc.Width = g_nClientWidth;
        desc.BufferDesc.Height = g_nClienthHeight;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferDesc.RefreshRate.Numerator = 60;
        desc.BufferDesc.RefreshRate.Denominator = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow = g_hWnd;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Windowed = TRUE;
        hr = pIDXGIFactory->CreateSwapChain(g_pd3dDevice, &desc, &g_pSwapChain);
    }

    pDXGIDevice->Release();
    pDXGIAdapter->Release();
    pIDXGIFactory->Release();

    if (FAILED(hr)) {
        ShowError(_T("Failed to create swap chain."));
        return hr;
    }

    {
        ID3D11Texture2D *pBackBuffer = nullptr;
        D3D11_TEXTURE2D_DESC desc;
        hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
        if (FAILED(hr)) {
            ShowError(_T("Failed to get back buffer."));
            return hr;
        }
        pBackBuffer->GetDesc(&desc);
        hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRTV);
        SAFE_RELEASE(pBackBuffer);
        if (FAILED(hr)) {
            ShowError(_T("Failed to create render target view."));
            return hr;
        }
    }

    {
        RECT rc;
        GetClientRect(g_hWnd, &rc);
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = rc.right - rc.left;
        desc.Height = rc.bottom - rc.top;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, &g_pDepthStencil);
        if (FAILED(hr)) {
            ShowError(_T("Failed to create depth stencil."));
            return hr;
        }

        D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
        ZeroMemory(&descDSV, sizeof(descDSV));
        descDSV.Format = desc.Format;
        descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        descDSV.Texture2D.MipSlice = 0;
        hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
        if (FAILED(hr)) {
            ShowError(_T("Failed to create depth stencil view."));
            return hr;
        }
    }

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRTV, g_pDepthStencilView);

    {
        D3D11_RASTERIZER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.FrontCounterClockwise = FALSE;
        desc.DepthClipEnable = TRUE;
        hr = g_pd3dDevice->CreateRasterizerState(&desc, &g_pRS);
        if (FAILED(hr)){
            ShowError(_T("Failed to create rasterizer state."));
            return hr;
        }
    }

    g_pImmediateContext->RSSetState(g_pRS);

    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)g_nClientWidth;
    vp.Height = (FLOAT)g_nClienthHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0;
    g_pImmediateContext->RSSetViewports(1, &vp);

    return S_OK;
}

HRESULT MakeShaders(void) {
    HRESULT hr;
    ID3DBlob *pVertexShaderBuffer = nullptr;
    ID3DBlob *pPixelShaderBuffer = nullptr;
    ID3DBlob *pError = nullptr;

    LPTSTR fileName = _T("Basic_3D_Light.fx");

    DWORD dwShaderFlags = 0;
#ifdef DEBUG
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif
    hr = D3DX11CompileFromFile(fileName, NULL, NULL, "VS", "vs_4_0_level_9_1",
        dwShaderFlags, 0, NULL, &pVertexShaderBuffer, &pError, NULL);
    if (FAILED(hr)) {
        ShowError(_T("Failed to compile vertex shader."));
        SAFE_RELEASE(pError);
        return hr;
    }
    hr = D3DX11CompileFromFile(fileName, NULL, NULL, "PS", "ps_4_0_level_9_1",
        dwShaderFlags, 0, NULL, &pPixelShaderBuffer, &pError, NULL);
    if (FAILED(hr)) {
        ShowError(_T("Failed to compile pixel shader."));
        SAFE_RELEASE(pVertexShaderBuffer);
        SAFE_RELEASE(pError);
        return hr;
    }
    SAFE_RELEASE(pError);

    hr = g_pd3dDevice->CreateVertexShader(
        pVertexShaderBuffer->GetBufferPointer(),
        pVertexShaderBuffer->GetBufferSize(),
        NULL, &g_pVertexShader);
    if (FAILED(hr)) {
        ShowError(_T("Failed to create vertex shader."));
        SAFE_RELEASE(pVertexShaderBuffer);
        SAFE_RELEASE(pPixelShaderBuffer);
        return hr;
    }
    hr = g_pd3dDevice->CreatePixelShader(
        pPixelShaderBuffer->GetBufferPointer(),
        pPixelShaderBuffer->GetBufferSize(),
        NULL, &g_pPixelShader);
    if (FAILED(hr)) {
        ShowError(_T("Failed to create pixel shader."));
        SAFE_RELEASE(pVertexShaderBuffer);
        SAFE_RELEASE(pPixelShaderBuffer);
        return hr;
    }

    {
        D3D11_INPUT_ELEMENT_DESC desc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        UINT numElements = ARRAYSIZE(desc);
        hr = g_pd3dDevice->CreateInputLayout(desc, numElements,
            pVertexShaderBuffer->GetBufferPointer(),
            pVertexShaderBuffer->GetBufferSize(),
            &g_pInputLayout);
        SAFE_RELEASE(pVertexShaderBuffer);
        SAFE_RELEASE(pPixelShaderBuffer);
        if (FAILED(hr)) {
            ShowError(_T("Failed to create input layout."));
            return hr;
        }
    }

    {
        D3D11_BUFFER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.ByteWidth = sizeof(CBNeverChanges);
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        hr = g_pd3dDevice->CreateBuffer(&desc, NULL, &g_pCBNeverChanges);
        if (FAILED(hr)){
            ShowError(_T("Failed to create buffer for constant buffer."));
            return hr;
        }
    }

    CBNeverChanges cbNeverChanges;
    XMMATRIX mScreen = XMMatrixIdentity();
    mScreen._11 = 2.0f / g_nClientWidth;
    mScreen._22 = -2.0f / g_nClienthHeight;
    mScreen._41 = -1.0f;
    mScreen._42 = 1.0f;
    cbNeverChanges.matView = XMMatrixTranspose(mScreen);
    g_pImmediateContext->UpdateSubresource(g_pCBNeverChanges, 0, NULL, &cbNeverChanges, 0, 0);

    return S_OK;
}

int LoadTexture(LPTSTR szFileName, TEX_PICTURE *pTexPic,
    int nWidth, int nHeight, int nTexWidth, int nTexHeight)
{
    HRESULT hr;

    D3DX11_IMAGE_LOAD_INFO info;
    ZeroMemory(&info, sizeof(info));
    info.Width = nTexWidth;
    info.Height = nTexHeight;
    info.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    info.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, szFileName, &info,
        nullptr, &(pTexPic->pSRViewTexture), nullptr);
    if (FAILED(hr)) {
        ShowError(_T("Failed to create shader resource view from file."));
        return hr;
    }

    ID3D11Texture2D *pTexture;
    pTexPic->pSRViewTexture->GetResource((ID3D11Resource**)&pTexture);
    pTexture->GetDesc(&(pTexPic->tdDesc));
    pTexture->Release();

    pTexPic->nWidth = nWidth;
    pTexPic->nHeight = nHeight;

    return S_OK;
}

int InitDrawModes(void)
{
    HRESULT hr;

    {
        D3D11_BLEND_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.AlphaToCoverageEnable = FALSE;
        desc.IndependentBlendEnable = FALSE;
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        hr = g_pd3dDevice->CreateBlendState(&desc, &g_pbsAlphaBlend);
        if (FAILED(hr)) {
            ShowError(_T("Failed to create blend state"));
            return hr;
        }
    }

    {
        D3D11_SAMPLER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        desc.MaxLOD = D3D11_FLOAT32_MAX;
        hr = g_pd3dDevice->CreateSamplerState(&desc, &g_pSamplerState);
        if (FAILED(hr)) {
            ShowError(_T("Failed to create sampler state"));
            return hr;
        }
    }

    return S_OK;
}

HRESULT InitGeometry(void)
{
    HRESULT hr;

    D3D11_BUFFER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = sizeof(CUSTOMVERTEX) * MAX_BUFFER_VERTEX;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA SubResourceData;
    SubResourceData.pSysMem = g_cvVertices;
    SubResourceData.SysMemPitch = 0;
    SubResourceData.SysMemSlicePitch = 0;
    hr = g_pd3dDevice->CreateBuffer(&desc, &SubResourceData, &g_pVertexBuffer);
    if (FAILED(hr)) {
        ShowError(_T("Failed to create vertex buffer"));
        return hr;
    }

    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = sizeof(WORD) * MAX_BUFFER_INDEX;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;

    SubResourceData.pSysMem = g_wIndices;
    hr = g_pd3dDevice->CreateBuffer(&desc, &SubResourceData, &g_pIndexBuffer);
    if (FAILED(hr)) {
        ShowError(_T("Failed to create index buffer"));
        return hr;
    }

    g_tTexture.pSRViewTexture = nullptr;
    //hr = LoadTexture(_T("4.bmp"), &g_tTexture, 1152, 576, 1024, 512);
    //if (FAILED(hr)) {
    //    ShowError(_T("Failed to load texture"));
    //    return hr;
    //}

    return S_OK;
}

void Cleanup(void)
{
    SAFE_RELEASE(g_tTexture.pSRViewTexture);
    SAFE_RELEASE(g_pVertexBuffer);
    SAFE_RELEASE(g_pIndexBuffer);

    SAFE_RELEASE(g_pSamplerState);
    SAFE_RELEASE(g_pbsAlphaBlend);
    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_pVertexShader);
    SAFE_RELEASE(g_pCBNeverChanges);
    SAFE_RELEASE(g_pRS);

    if (g_pImmediateContext) {
        g_pImmediateContext->ClearState();
        g_pImmediateContext->Flush();
    }

    SAFE_RELEASE(g_pRTV);
    SAFE_RELEASE(g_pDepthStencil);
    SAFE_RELEASE(g_pDepthStencilView);

    if (g_pSwapChain) {
        g_pSwapChain->SetFullscreenState(FALSE, 0);
    }
    SAFE_RELEASE(g_pSwapChain);

    SAFE_RELEASE(g_pImmediateContext);
    SAFE_RELEASE(g_pd3dDevice);
}


LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void DrawIndexed3DPolygonsTex(CUSTOMVERTEX *pVertices, int nVertexNum, WORD *pIndices, int nIndexNum)
{
    CopyMemory(&g_cvVertices[0], pVertices, sizeof(CUSTOMVERTEX) * nVertexNum);
    for (auto i = 0; i < nIndexNum; ++i) {
        g_wIndices[g_nIndexNum + i] = *(pIndices + i) + g_nVertexNum;
    }

    g_nVertexNum += nVertexNum;
    g_nIndexNum += nIndexNum;

    g_pNowTexture = g_tTexture.pSRViewTexture;
}

XMMATRIX CreateWorldMatrix(void)
{
    static float fAngleX = 0.0f;
    float fAngleY = -XM_2PI * (float)(timeGetTime() % 3000) / 3000.0f;

    if (GetAsyncKeyState(VK_UP)) {
        fAngleX += ROT_SPEED;
    }
    if (GetAsyncKeyState(VK_DOWN)) {
        fAngleX -= ROT_SPEED;
    }

    auto matRotY = XMMatrixRotationY(fAngleY);
    auto matRotX = XMMatrixRotationX(fAngleX);

    return matRotY * matRotX;
}

void DrawChangingPictures(void)
{
    CUSTOMVERTEX Vertices[(CORNER_NUM + 1) * (CORNER_NUM / 2 + 1)];
    WORD wIndices[CORNER_NUM * CORNER_NUM * 3];

    auto fAngleDelta = XM_2PI / CORNER_NUM;
    auto nIndex = 0;
    auto fTheta = 0.0f;

    for (auto i = 0; i < CORNER_NUM / 2 + 1; ++i) {
        auto fPhi = 0.0f;
        for (auto j = 0; j < CORNER_NUM + 1; ++j) {
            auto x = R * sinf(fTheta) * cosf(fPhi);
            auto y = R * cosf(fTheta);
            auto z = R * sinf(fTheta) * sinf(fPhi);
            Vertices[nIndex].v4Pos = XMFLOAT4(x, y, z, 1.0f);
            Vertices[nIndex].v3Normal = XMFLOAT3(x / R, y / R, z / R);
            nIndex++;
            fPhi += fAngleDelta;
        }
        fTheta += fAngleDelta;
    }

    nIndex = 0;
    for (auto i = 0; i < CORNER_NUM / 2; ++i) {
        auto nIndexY = i * (CORNER_NUM + 1);
        for (auto j = 0; j < CORNER_NUM; ++j) {
            wIndices[nIndex + 0] = nIndexY + j;
            wIndices[nIndex + 1] = nIndexY + (CORNER_NUM + 1) + j;
            wIndices[nIndex + 2] = nIndexY + j + 1;
            nIndex += 3;
            wIndices[nIndex + 0] = nIndexY + j + 1;
            wIndices[nIndex + 1] = nIndexY + (CORNER_NUM + 1) + j;
            wIndices[nIndex + 2] = nIndexY + (CORNER_NUM + 1) + j + 1;
            nIndex += 3;
        }
    }

    DrawIndexed3DPolygonsTex(Vertices, ARRAYSIZE(Vertices),
        wIndices, ARRAYSIZE(wIndices));
}

void FlushDrawingPictures(void)
{
    HRESULT hr;

    if ((g_nVertexNum > 0) /*&& g_pNowTexture*/) {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = g_pImmediateContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (SUCCEEDED(hr)){
            CopyMemory(mappedResource.pData, &g_cvVertices[0], sizeof(CUSTOMVERTEX) * g_nVertexNum);
            g_pImmediateContext->Unmap(g_pVertexBuffer, 0);

            hr = g_pImmediateContext->Map(g_pIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
            if (SUCCEEDED(hr)) {
                CopyMemory(mappedResource.pData, &g_wIndices[0], sizeof(WORD) * g_nIndexNum);
                g_pImmediateContext->Unmap(g_pIndexBuffer, 0);

                //g_pImmediateContext->PSSetShaderResources(0, 1, &g_pNowTexture);
                g_pImmediateContext->DrawIndexed(g_nIndexNum, 0, 0);
            }
        }
    }

    g_nVertexNum = 0;
    g_nIndexNum = 0;
}

void Render(void) {
    XMFLOAT4 v4Color = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    g_pImmediateContext->ClearRenderTargetView(g_pRTV, (float*)&v4Color);
    
    g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerState);
    g_pImmediateContext->RSSetState(g_pRS);

    UINT nStrides = sizeof(CUSTOMVERTEX);
    UINT nOffsets = 0;
    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &nStrides, &nOffsets);
    g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pImmediateContext->IASetInputLayout(g_pInputLayout);

    g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
    g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBNeverChanges);
    g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);

    XMMATRIX matWorld = CreateWorldMatrix();

    XMVECTOR Eye = XMVectorSet(0.0f, 3.0f, -5.0f, 0.0f);
    XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX matView = XMMatrixLookAtLH(Eye, At, Up);

    XMMATRIX matProjection = XMMatrixPerspectiveFovLH(XM_PIDIV4, VIEW_WIDTH / (FLOAT)VIEW_HEIGHT, 0.01f, 100.0f);

    CBNeverChanges cbNeverChanges;
    cbNeverChanges.matView = XMMatrixTranspose(matWorld * matView * matProjection);
    cbNeverChanges.matWorld = XMMatrixTranspose(matWorld);
    cbNeverChanges.v3Light = XMFLOAT3(-1.0f / sqrtf(3.0f), 1.0f / sqrtf(3.0f), -1.0f / sqrtf(3.0f));
    cbNeverChanges.fAmbient = 0.2f;
    cbNeverChanges.fDirectional = 0.8f;
    g_pImmediateContext->UpdateSubresource(g_pCBNeverChanges, 0, NULL, &cbNeverChanges, 0, 0);

    g_pImmediateContext->OMSetBlendState(NULL, NULL, 0xFFFFFFFF);
    DrawChangingPictures();

    FlushDrawingPictures();
}

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE, LPTSTR, int)
{
    LARGE_INTEGER nNowTime, nLastTime;
    LARGE_INTEGER nTimeFreq;

    g_nClientWidth = VIEW_WIDTH;
    g_nClienthHeight = VIEW_HEIGHT;

    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
        GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
        _T("D3D sample"), NULL
    };
    RegisterClassEx(&wc);

    RECT rc;
    SetRect(&rc, 0, 0, g_nClientWidth, g_nClienthHeight);
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hWnd = CreateWindow(_T("D3D sample"), _T("Textures_1_1"),
        WS_OVERLAPPEDWINDOW, 100, 20, rc.right - rc.left, rc.bottom - rc.top,
        GetDesktopWindow(), NULL, wc.hInstance, NULL);

    if (SUCCEEDED(InitD3D())
        && SUCCEEDED(MakeShaders())
        && SUCCEEDED(InitDrawModes())
        && SUCCEEDED(InitGeometry()))
    {
        ShowWindow(g_hWnd, SW_SHOWDEFAULT);
        UpdateWindow(g_hWnd);

        QueryPerformanceFrequency(&nTimeFreq);
        QueryPerformanceCounter(&nLastTime);

        MSG msg;
        ZeroMemory(&msg, sizeof(msg));
        while (msg.message != WM_QUIT) {
            Render();
            do {
                if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                QueryPerformanceCounter(&nNowTime);
            
            } while (((nNowTime.QuadPart - nLastTime.QuadPart) < (nTimeFreq.QuadPart / 90))
                && (msg.message != WM_QUIT));


            while (((nNowTime.QuadPart - nLastTime.QuadPart) < (nTimeFreq.QuadPart / 60))
                && (msg.message != WM_QUIT))
            {
                QueryPerformanceCounter(&nNowTime);
            }

            nLastTime = nNowTime;
            g_pSwapChain->Present(0, 0);
        }
    }

    Cleanup();
    UnregisterClass(_T("D3D sample"), wc.hInstance);
    return 0;
}
