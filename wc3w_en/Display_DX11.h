/*
The MIT License (MIT)
Copyright © 2024 Matt Wells

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the “Software”), to deal in the
Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <d3d11.h>
#include <d3d11_2.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXPackedVector.h>
#include <dxgi.h>
#include <dxgi1_2.h>

#include <iostream>
#include <string>
#include <algorithm>

// Link library dependencies
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")


#define RGBA_32BIT(a,r,g,b) (a<<24) | (r<<16) | (g<<8) | (b)


__declspec(align(16))struct VERTEX_BASE {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT2 texUV;
    DirectX::XMFLOAT3 Normal;
};

__declspec(align(16)) struct MATRIX_DATA {
    DirectX::XMMATRIX World;
    DirectX::XMMATRIX WorldViewProjection;
};


__declspec(align(16)) struct PALETTE_BUFF_DATA {
    DirectX::XMFLOAT4 pal_colour;
};

__declspec(align(16)) struct COLOUR_BUFF_DATA {
    DirectX::XMFLOAT4 colour_val;
    DirectX::XMFLOAT4 colour_opt;
};



// Shader data
extern ID3D11VertexShader* pd3dVertexShader_Main;
extern ID3D11InputLayout* pd3dVS_InputLayout_Main;

extern ID3D11PixelShader* pd3d_PS_Basic_Tex_32;
extern ID3D11PixelShader* pd3d_PS_Basic_Tex_8;
extern ID3D11PixelShader* pd3d_PS_Basic_Tex_8_masked;
extern ID3D11PixelShader* pd3d_PS_Greyscale_Tex_32;
extern ID3D11PixelShader* pd3d_PS_Gamma_Tex_32;

extern ID3D11SamplerState* pd3dPS_SamplerState_Point;
extern ID3D11SamplerState* pd3dPS_SamplerState_Linear;

extern ID3D11BlendState* pBlendState_Zero;
extern ID3D11BlendState* pBlendState_One;
extern ID3D11BlendState* pBlendState_Two;
extern ID3D11BlendState* pBlendState_Three;
extern ID3D11BlendState* pBlendState_Four;


extern ID3D11Buffer* pVB_Quad_IndexBuffer;
extern ID3D11Buffer* pVB_Quad_Line_IndexBuffer;

//The maximum supported texture width or height for your graphics card.
extern UINT max_texDim;


enum class SCALE_TYPE : LONG {
    none = -1,
    fit = 0,
    fill = 1,
    fit_best = 2,
    fit_height = 3,
};

enum class PRESENT_TYPE : LONG {
    gui = 0,
    movie = 1,
    space = 2,
    //video_hd = 3,
};


ID3D11Device* GetD3dDevice();
ID3D11DeviceContext* GetD3dDeviceContext();
ID3D11RenderTargetView* GetRenderTargetView();
ID3D11DepthStencilView* GetDepthStencilView();
ID3D11DepthStencilState* GetDepthStencilState();

void Set_ViewPort(long width, long height);
void Set_ViewPort(float x, float y, float width, float height);

bool SetScreenProjectionMatrix_XM(DWORD width, DWORD height);
DirectX::XMMATRIX* GetScreenProjectionMatrix_XM();

bool CreateQuadVB(ID3D11Device* pD3DDev, unsigned int width, unsigned int height, ID3D11Buffer** plpVB);

bool Shader_Main_Setup();
void Shader_Main_Destroy();

void Shader_SetPaletteData(DirectX::XMFLOAT4 pal_data);

class BUFFER_DX {
public:
    BUFFER_DX(UINT in_numConstantBuffers, bool in_isUpdatedOften, UINT in_ByteWidth) {
        ByteWidth = in_ByteWidth;
        isUpdatedOften = in_isUpdatedOften;
        Create_ConstantBuffers(in_numConstantBuffers);

    }
    BUFFER_DX(bool in_isUpdatedOften, UINT in_ByteWidth) {
        ByteWidth = in_ByteWidth;
        isUpdatedOften = in_isUpdatedOften;
        numConstantBuffers = 0;
        lpd3dConstantBuffers = nullptr;
    }
    ~BUFFER_DX() {
        if (lpd3dConstantBuffers == nullptr)
            return;
        ID3D11Buffer* pTemp = nullptr;
        for (UINT i = 0; i < numConstantBuffers; i++) {
            pTemp = lpd3dConstantBuffers[i];
            if (pTemp != nullptr)
                pTemp->Release();
            pTemp = nullptr;
        }
        delete[] lpd3dConstantBuffers;
        lpd3dConstantBuffers = nullptr;

    }
    void UpdateData(ID3D11DeviceContext* pD3DDevContext, UINT buffer_num, const void* data) {
        if (lpd3dConstantBuffers == nullptr)
            return;
        if (buffer_num >= numConstantBuffers)
            return;
        if (lpd3dConstantBuffers[buffer_num] == nullptr)
            return;
        if (isUpdatedOften) {

            D3D11_MAPPED_SUBRESOURCE mappedResource;
            HRESULT result;
            result = pD3DDevContext->Map(lpd3dConstantBuffers[buffer_num], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
            if (FAILED(result))
            {
                Debug_Info_Error("BUFFER_DX - Failed to Map Constant buffers.");
                return;
            }
            memcpy(mappedResource.pData, data, ByteWidth);
            pD3DDevContext->Unmap(lpd3dConstantBuffers[buffer_num], 0);
        }
        else
            pD3DDevContext->UpdateSubresource(lpd3dConstantBuffers[buffer_num], 0, nullptr, data, 0, 0);

    };
    ID3D11Buffer* GetBuffer(UINT buffer_num) {
        if (lpd3dConstantBuffers == nullptr)
            return nullptr;
        if (buffer_num >= numConstantBuffers)
            return nullptr;
        return lpd3dConstantBuffers[buffer_num];
    };
    void SetForRenderPS(ID3D11DeviceContext* pD3DDevContext, UINT buffer_num, UINT constPos) {
        if (lpd3dConstantBuffers == nullptr)
            return;
        if (buffer_num >= numConstantBuffers)
            return;
        pD3DDevContext->PSSetConstantBuffers(constPos, 1, &lpd3dConstantBuffers[buffer_num]);
    }
    void SetForRenderVS(ID3D11DeviceContext* pD3DDevContext, UINT buffer_num, UINT constPos) {
        if (lpd3dConstantBuffers == nullptr)
            return;
        if (buffer_num >= numConstantBuffers)
            return;
        pD3DDevContext->VSSetConstantBuffers(constPos, 1, &lpd3dConstantBuffers[buffer_num]);
    }
protected:
    bool Create_ConstantBuffers(UINT in_numConstantBuffers) {
        if (lpd3dConstantBuffers != nullptr)
            return false;
        lpd3dConstantBuffers = new ID3D11Buffer * [in_numConstantBuffers];
        ID3D11Device* pD3DDev = GetD3dDevice();
        D3D11_BUFFER_DESC constantBufferDesc;
        ZeroMemory(&constantBufferDesc, sizeof(D3D11_BUFFER_DESC));

        constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constantBufferDesc.ByteWidth = ByteWidth;

        if (isUpdatedOften) {
            constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        }
        else {
            constantBufferDesc.CPUAccessFlags = 0;
            constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        }
        constantBufferDesc.MiscFlags = 0;
        constantBufferDesc.StructureByteStride = 0;

        for (UINT i = 0; i < in_numConstantBuffers; i++) {
            HRESULT hr = pD3DDev->CreateBuffer(&constantBufferDesc, nullptr, &lpd3dConstantBuffers[i]);
            if (FAILED(hr)) {
                Debug_Info_Error("BUFFER_DX - Failed to CreateConstantBuffer_MatrixData.");
                return false;
            }
            numConstantBuffers = i + 1;
        }
        return true;
    };
    void DestroyConstantBuffers() {
        if (lpd3dConstantBuffers != nullptr) {
            ID3D11Buffer* pTemp = nullptr;
            for (UINT i = 0; i < numConstantBuffers; i++) {
                pTemp = lpd3dConstantBuffers[i];
                if (pTemp != nullptr)
                    pTemp->Release();
                pTemp = nullptr;
            }
            delete[] lpd3dConstantBuffers;
            lpd3dConstantBuffers = nullptr;
        }
    };
    UINT ByteWidth;
    UINT numConstantBuffers;
    bool isUpdatedOften;
    ID3D11Buffer** lpd3dConstantBuffers;
private:
};




class BASE_BASE_DX {
public:
    BASE_BASE_DX() {
        width = 0;
        height = 0;
        size = 0;
        pD3DDev = GetD3dDevice();
        pD3DDevContext = GetD3dDeviceContext();
    }
    ~BASE_BASE_DX() {
        pD3DDev = nullptr;
        pD3DDevContext = nullptr;
    };
    void SetBaseDimensions(DWORD in_width, DWORD in_height) {
        width = in_width;
        height = in_height;
        size = width * height;
    }
    DWORD GetWidth() const{
        return width;
    };
    DWORD GetHeight() const {
        return height;
    };
protected:
    ID3D11Device* pD3DDev;
    ID3D11DeviceContext* pD3DDevContext;
    DWORD width;
    DWORD height;
    DWORD size;
private:
};


class BASE_POSITION_DX : virtual public BASE_BASE_DX, public BUFFER_DX {
public:
    BASE_POSITION_DX(float inX, float inY, UINT in_numConstantBuffers, bool in_isUpdatedOften) :
        BUFFER_DX(in_numConstantBuffers, in_isUpdatedOften, sizeof(MATRIX_DATA)),
        x(inX),
        y(inY),
        z(0.0f),
        scaleX(1.0f),
        scaleY(1.0f),
        isPositionScaled(false)
    {

    };
    BASE_POSITION_DX(float inX, float inY, bool in_isUpdatedOften) :
        BUFFER_DX(in_isUpdatedOften, sizeof(MATRIX_DATA)),
        x(inX),
        y(inY),
        z(0.0f),
        scaleX(1.0f),
        scaleY(1.0f),
        isPositionScaled(false)
    {

    };
    ~BASE_POSITION_DX() {
    };
    void SetPosition(float inX, float inY) {
        x = inX, y = inY;
        SetMatrices();
    };
    void GetPosition(float* pX, float* pY) const {
        if (pX)
            *pX = x;
        if (pY)
            *pY = y;
    };
    void SetScale(float inScaleX, float inScaleY) {
        if (scaleX == inScaleX && scaleY == inScaleY)
            return;
        scaleX = inScaleX;
        scaleY = inScaleY;
        SetMatrices();
    };
    void RefreshMatrices() {
        SetMatrices();
    };
    void ScalePosition(bool in_isPositionScaled) {
        isPositionScaled = in_isPositionScaled;
    };
    void GetScaledPixelDimensions(float* p_x, float* p_y) const {
        if (p_x)
            *p_x = scaleX;
        if (p_y)
            *p_y = scaleY;
    }
protected:
    float x;
    float y;
    float z;
    float scaleX;
    float scaleY;
    bool isPositionScaled;

    virtual void SetMatrices() {
        MATRIX_DATA posData{};
        DirectX::XMMATRIX xmManipulation{};
        DirectX::XMMATRIX xmScaling{};
        if (isPositionScaled)
            xmManipulation = DirectX::XMMatrixTranslation(x * scaleX, y * scaleY, 0);
        else
            xmManipulation = DirectX::XMMatrixTranslation(x, y, 0);
        xmScaling = DirectX::XMMatrixScaling(scaleX, scaleY, 1.0f);
        posData.World = DirectX::XMMatrixMultiply(xmScaling, xmManipulation);
        DirectX::XMMATRIX* pOrtho2D_SCRN = GetScreenProjectionMatrix_XM();
        posData.WorldViewProjection = DirectX::XMMatrixMultiply(posData.World, *pOrtho2D_SCRN);
        UpdatePositionData(0, &posData);
    };
    void UpdatePositionData(UINT buffer_num, MATRIX_DATA* posData) {
        UpdateData(pD3DDevContext, buffer_num, posData);
    }
    void SetPositionRender(UINT buffer_num) {
        if (lpd3dConstantBuffers == nullptr)
            return;
        if (buffer_num >= numConstantBuffers)
            return;
        pD3DDevContext->VSSetConstantBuffers(0, 1, &lpd3dConstantBuffers[buffer_num]);
    }
private:
};




class BASE_VERTEX_DX : virtual public BASE_BASE_DX {
public:
    BASE_VERTEX_DX() :
        pd3dVB(nullptr)
    {
    };
    ~BASE_VERTEX_DX() {
        DestroyVerticies();
    };
    void GetVertexBuffer(ID3D11Buffer** ppVB) {
        if (ppVB)
            *ppVB = pd3dVB;
    };
    bool isVertex() {
        if (pd3dVB)
            return true;
        return false;
    }
protected:
    ID3D11Buffer* pd3dVB;

    virtual bool CreateVerticies() {
        if (width == 0 || height == 0)
            return false;
        return CreateQuadVB(pD3DDev, width, height, &pd3dVB);
    };
    virtual bool CreateVerticies(DWORD in_width, DWORD in_height) {
        if (in_width == 0 || in_height == 0)
            return false;
        return CreateQuadVB(pD3DDev, in_width, in_height, &pd3dVB);
    };
    virtual void DestroyVerticies() {
        if (pd3dVB != nullptr) {
            pd3dVB->Release();
            pd3dVB = nullptr;
        }
    };
private:
};


class BASE_TEXTURE_DX : virtual public BASE_BASE_DX {
public:
    BASE_TEXTURE_DX(DWORD in_bgColour) :
        pTex(nullptr),
        pTex_Staging(nullptr),
        pTex_shaderResourceView(nullptr),
        pTex_renderTargetView(nullptr),
        isRenderTarget(false),
        hasStagingTexture(false),
        dxgiFormat(DXGI_FORMAT_UNKNOWN),
        pixelWidth(0)
    {
        SetBackGroungColour(in_bgColour);
    };
    ~BASE_TEXTURE_DX() {
        DestroyTexture();
    };
    void SetBackGroungColour(DWORD in_bgColour) {
        bg_Colour = in_bgColour;
        bg_Colour_f[3] = ((bg_Colour & 0xFF000000) >> 24) / 256.0f;
        bg_Colour_f[2] = ((bg_Colour & 0x00FF0000) >> 16) / 256.0f;
        bg_Colour_f[1] = ((bg_Colour & 0x0000FF00) >> 8) / 256.0f;
        bg_Colour_f[0] = ((bg_Colour & 0x000000FF)) / 256.0f;
    }
    ID3D11ShaderResourceView* GetShaderResourceView() {
        return pTex_shaderResourceView;
    };
    void SetRenderTarget(ID3D11DepthStencilView* depthStencilView) {
        pD3DDevContext->OMSetRenderTargets(1, &pTex_renderTargetView, depthStencilView);
        Set_ViewPort(width, height);
        return;
    };
    void ClearRenderTarget(ID3D11DepthStencilView* depthStencilView) {
        pD3DDevContext->ClearRenderTargetView(pTex_renderTargetView, bg_Colour_f);
        pD3DDevContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
        return;
    }
    void ClearRenderTarget(ID3D11DepthStencilView* depthStencilView, float color[4]) {
        pD3DDevContext->ClearRenderTargetView(pTex_renderTargetView, color);
        pD3DDevContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
        return;
    }
    void SetRenderTargetAndViewPort(ID3D11DepthStencilView* depthStencilView) {
        pD3DDevContext->OMSetRenderTargets(1, &pTex_renderTargetView, depthStencilView);
        Set_ViewPort(width, height);
    }
    bool isTexture() {
        if (pTex)
            return true;
        return false;
    }
    bool GetOrthoMatrix(DirectX::XMMATRIX* pOrtho2D) {
        if (!pOrtho2D)
            return false;
        *pOrtho2D = DirectX::XMMatrixOrthographicOffCenterLH(0.0f, (float)(width), (float)(height), 0.0f, -1000.5f, 1000.5f);
        return true;
    };
    ID3D11Texture2D* GetTexture() {
        return pTex;
    };
    UINT GetPixelWidth() const {
        return pixelWidth;
    };
protected:
    ID3D11Texture2D* pTex;
    ID3D11Texture2D* pTex_Staging;
    ID3D11ShaderResourceView* pTex_shaderResourceView;
    ID3D11RenderTargetView* pTex_renderTargetView;
    UINT pixelWidth;
    DWORD bg_Colour;
    float bg_Colour_f[4];
    bool isRenderTarget;
    bool hasStagingTexture;
    virtual bool CreateTexture() {

        if (width == 0 || height == 0)
            return false;
        if (pTex)
            return true;
        D3D11_TEXTURE2D_DESC textureDesc;
        HRESULT result = S_OK;
        ZeroMemory(&textureDesc, sizeof(textureDesc));
        //Setup the render target texture description.
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = dxgiFormat;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (isRenderTarget)
            textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;
        //Create the texture.
        result = pD3DDev->CreateTexture2D(&textureDesc, nullptr, &pTex);
        if (FAILED(result))
            return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
        //Setup the description of the shader resource view.
        shaderResourceViewDesc.Format = textureDesc.Format;
        shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
        shaderResourceViewDesc.Texture2D.MipLevels = 1;

        //Create the shader resource view.
        result = pD3DDev->CreateShaderResourceView(pTex, &shaderResourceViewDesc, &pTex_shaderResourceView);
        if (FAILED(result)) {
            DestroyTexture();
            return false;
        }
        if (isRenderTarget) {
            D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{};
            //Setup the description of the render target view.
            renderTargetViewDesc.Format = dxgiFormat;
            renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            renderTargetViewDesc.Texture2D.MipSlice = 0;

            //Create the render target view.
            result = pD3DDev->CreateRenderTargetView(pTex, &renderTargetViewDesc, &pTex_renderTargetView);
            if (FAILED(result)) {
                DestroyTexture();
                return false;
            }
        }
        if (hasStagingTexture) {
            textureDesc.Usage = D3D11_USAGE_STAGING;
            textureDesc.BindFlags = 0;
            textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;// 0;
            result = pD3DDev->CreateTexture2D(&textureDesc, nullptr, &pTex_Staging);
            if (FAILED(result)) {
                DestroyTexture();
                return false;
            }
        }
        return true;
    };
    virtual bool Texture_Initialize(DXGI_FORMAT in_dxgiFormat, bool in_isRenderTarget, bool in_hasStagingTexture) {
        SetPixelFormat(in_dxgiFormat);
        isRenderTarget = in_isRenderTarget;
        hasStagingTexture = in_hasStagingTexture;
        return CreateTexture();

    };
    virtual void DestroyTexture() {
        if (pTex)
            pTex->Release();
        pTex = nullptr;
        if (pTex_Staging)
            pTex_Staging->Release();
        pTex_Staging = nullptr;
        if (pTex_shaderResourceView)
            pTex_shaderResourceView->Release();
        pTex_shaderResourceView = nullptr;
        if (pTex_renderTargetView)
            pTex_renderTargetView->Release();
        pTex_renderTargetView = nullptr;
    };
    void SetPixelFormat(DXGI_FORMAT in_dxgiFormat) {
        dxgiFormat = in_dxgiFormat;
        if (dxgiFormat == DXGI_FORMAT_R8_UNORM)
            pixelWidth = 1;
        else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM)
            pixelWidth = 2;
        else {
            dxgiFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
            pixelWidth = 4;
        }
    }
private:
    DXGI_FORMAT dxgiFormat;

};

/*
class BASE_TEXTURE_STAGING : public BASE_TEXTURE_DX {
public:
    BASE_TEXTURE_STAGING(DWORD in_bgColour) :
        BASE_TEXTURE_DX(in_bgColour),
        isLocked(false),
        lockType(D3D11_MAP_READ)
    {

    };
    ~BASE_TEXTURE_STAGING() {

    };
    HRESULT Lock(void** data, UINT* p_pitch, D3D11_MAP MapType) {
        if (!pTex_Staging || !pTex)
            return -1;
        if (isLocked)//already locked
            return -1;
        if (width == 0 || height == 0)
            return -1;
        HRESULT result;
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        lockType = MapType;
        result = pD3DDevContext->Map(pTex_Staging, 0, lockType, 0, &mappedResource);
        *data = (void*)mappedResource.pData;
        *p_pitch = mappedResource.RowPitch;
        if (SUCCEEDED(result))
            isLocked = true;
        return result;
    };
    void Unlock(RECT* pRect) {
        if (!pTex_Staging || !pTex)
            return;
        if (!isLocked)
            return;
        pD3DDevContext->Unmap(pTex_Staging, 0);
        if (lockType == D3D11_MAP_READ) {//reading only
            isLocked = false;//reset lockType to unlocked
            return;
        }
        //update main texture
        isLocked = false;//reset lockType to unlocked
        if (pRect) {
            if (pRect->left > (LONG)width || pRect->right < 0 || pRect->top >(LONG)height || pRect->bottom < 0)
                return;

            D3D11_BOX sourceRegion;

            sourceRegion.left = pRect->left;
            sourceRegion.right = pRect->right;
            sourceRegion.top = pRect->top;
            sourceRegion.bottom = pRect->bottom;
            sourceRegion.front = 0;
            sourceRegion.back = 1;
            pD3DDevContext->CopySubresourceRegion(pTex, 0, pRect->left, pRect->top, 0, pTex_Staging, 0, &sourceRegion);
        }
        else
            pD3DDevContext->CopyResource(pTex, pTex_Staging);

    };
    void Clear_Staging(DWORD colour) {
        if (!pTex_Staging || !pTex)
            return;
        BYTE* pBackBuff = nullptr;
        UINT pitch = 0;
        if (SUCCEEDED(Lock((void**)&pBackBuff, &pitch, D3D11_MAP_WRITE))) {
            if (pixelWidth == 4) {
                DWORD size = pitch / 4 * height;
                DWORD colourOffset = 0;
                while (colourOffset < size) {
                    ((DWORD*)pBackBuff)[colourOffset] = colour;
                    colourOffset++;
                }
            }
            else
                memset(pBackBuff, colour, pitch * height);
            Unlock(nullptr);
        }
    };
    void Clear_Staging() {
        Clear_Staging(bg_Colour);
    };
protected:
    virtual bool Texture_Initialize(DXGI_FORMAT in_dxgiFormat, bool in_isRenderTarget, bool in_hasStagingTexture) {
        SetPixelFormat(in_dxgiFormat);
        isRenderTarget = in_isRenderTarget;
        hasStagingTexture = in_hasStagingTexture;
        return CreateTexture();
    };
private:
    D3D11_MAP lockType;
    bool isLocked;
};
*/

class STORED_VIEW {//if in_on_create_and_destroy = true view will be stored on creation of this class and restored on destruction, otherwise call  StoreView and ReStoreView separately;
public:
    STORED_VIEW(bool in_on_create_and_destroy) {
        pD3DDevContext = GetD3dDeviceContext();
        pRenderTargetViews = nullptr;
        pDepthStencilView = nullptr;
        pCViewPorts = nullptr;
        pScissorRects = nullptr;
        numViewPorts = 0;
        on_create_and_destroy = in_on_create_and_destroy;
        if (on_create_and_destroy)
            StoreView();
    }
    ~STORED_VIEW() {
        if (on_create_and_destroy)
            ReStoreView();
        if (pRenderTargetViews)
            pRenderTargetViews->Release();
        pRenderTargetViews = nullptr;
        if (pDepthStencilView)
            pDepthStencilView->Release();
        pDepthStencilView = nullptr;
        if (pCViewPorts != nullptr) {
            delete[] pCViewPorts;
            pCViewPorts = nullptr;
        }
        if (pScissorRects != nullptr) {
            delete[] pScissorRects;
            pScissorRects = nullptr;
        }
        numViewPorts = 0;
    }
    void StoreView();
    void ReStoreView();
protected:
private:
    ID3D11DeviceContext* pD3DDevContext;
    ID3D11RenderTargetView* pRenderTargetViews;
    ID3D11DepthStencilView* pDepthStencilView;
    UINT numViewPorts;
    D3D11_VIEWPORT* pCViewPorts;
    D3D11_RECT* pScissorRects;
    bool on_create_and_destroy;
};



class PAL_DX : public BASE_TEXTURE_DX {
public:
    PAL_DX(UINT inSize, bool in_isVolatile) :
        BASE_TEXTURE_DX(0x00000000),
        isLocked(false),
        lockType(D3D11_MAP_READ)
    {
        SetBaseDimensions(inSize, 1);
        trans_offset = inSize+1;//set out of range by default;
        if (!Texture_Initialize(DXGI_FORMAT_B8G8R8A8_UNORM, false, true))
            Debug_Info_Error("PAL_DX - Failed BASE_TEXTURE_DX CreateTexture.");
    };
    ~PAL_DX() {
    };
    void SetPalEntries(BYTE* palette, DWORD offset, DWORD dwNumEntries) {
        if (offset < 0) {
            Debug_Info_Error("SetPalEntries fail: offset < 0");
            return;
        }
        if (offset + dwNumEntries > size) {
            Debug_Info_Error("SetPalEntries fail: offset + dwNumEntries > size offset:%d, dwNumEntries:%d", offset, dwNumEntries);
            return;
        }
        DWORD* paldata = nullptr;
        DWORD colour = 0;
        if (FAILED(Lock(&paldata, D3D11_MAP_WRITE))) {
            Debug_Info_Error("SetPalEntries fail: Lock");
            return;
        }
        DWORD colourOffset = offset;
        while (colourOffset < offset + dwNumEntries) {
            paldata[colourOffset] = RGBA_32BIT(0xFF, palette[0] << 2, palette[1] << 2, palette[2] << 2);
            //paldata[colourOffset] = RGBA_32BIT(0xFF, palette[0], palette[1], palette[2]);
            //paldata[colourOffset] = RGBA_32BIT(0xFF, 0xFF, 0xFF, 0xFF);
            if (colourOffset == trans_offset)//set palette[trans_offset] alpha to 0 - transparent
                paldata[colourOffset] = (paldata[colourOffset] & 0x00FFFFFF);
            palette += 3;
            colourOffset++;
        }
        Unlock(offset, dwNumEntries);
    };
    void SetPalEntry(int offset, BYTE r, BYTE g, BYTE b) {
        BYTE palette[3] = { r,g,b };
        SetPalEntries(palette, offset, 1);
    };
    bool GetPalEntries(DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpEntries) {
        if (lpEntries == nullptr)
            return false;
        if (dwBase > size - 1)
            return false;
        if (dwBase + dwNumEntries > size)
            return false;

        DWORD* paldata = nullptr;
        DWORD colour = 0;
        if (FAILED(Lock(&paldata, D3D11_MAP_READ)))
            return false;

        DWORD offset = 0;
        while (dwNumEntries) {
            lpEntries[offset].peFlags = (paldata[dwBase] >> 24) & 0x000000FF;
            lpEntries[offset].peBlue = (paldata[dwBase] >> 16) & 0x000000FF;
            lpEntries[offset].peGreen = (paldata[dwBase] >> 8) & 0x000000FF;
            lpEntries[offset].peRed = (paldata[dwBase]) & 0x000000FF;
            offset++;
            dwBase++;
            dwNumEntries--;
        }
        Unlock(0, 0);
        return true;
    };
    void SetTransparentOffset(DWORD offset) {
        if (trans_offset == offset)
            return;

        DWORD* paldata = nullptr;
        DWORD colour = 0;
        if (FAILED(Lock(&paldata, D3D11_MAP_WRITE))) {
            Debug_Info_Error("SetPalEntries fail: Lock");
            return;
        }
        if (trans_offset >= 0 && trans_offset < size)
            paldata[trans_offset] |= 0xFF000000;
        trans_offset = offset;
        if (trans_offset >= 0 && trans_offset < size)
            paldata[trans_offset] &= 0x00FFFFFF;
        

        Unlock(0, size);

    };
    //void SetTransparentOffset(DWORD offset) {
    //    trans_offset = offset;
    //};
    DWORD GetColour(UINT offset) {
        if (offset < size) {
            DWORD* paldata = nullptr;
            DWORD colour = 0;
            if (SUCCEEDED(Lock(&paldata, D3D11_MAP_READ)) && paldata != nullptr)
                colour = paldata[offset];
            Unlock(0, 0);
            return colour;
        }
        return 0;
    };
    HRESULT Lock(DWORD** paldata, D3D11_MAP MapType) {
        if (!pTex_Staging)
            return -1;
        if (isLocked)//already locked
            return -1;
        HRESULT result;
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        result = pD3DDevContext->Map(pTex_Staging, 0, MapType, 0, &mappedResource);
        *paldata = (DWORD*)mappedResource.pData;
        lockType = MapType;
        if (SUCCEEDED(result))
            isLocked = true;
        return result;

    };
    void Unlock(DWORD offset, DWORD dwNumEntries) {
        if (!pTex_Staging)
            return;
        if (!isLocked)
            return;
        pD3DDevContext->Unmap(pTex_Staging, 0);
        if (lockType == D3D11_MAP_READ) {//reading only
            isLocked = false;//reset lockType to unlocked
            return;
        }
        //update main texture
        isLocked = false;//reset lockType to unlocked
        if (offset < 0)
            return;
        if (offset + dwNumEntries > size)
            return;
        D3D11_BOX destRegion{};
        destRegion.left = offset;
        destRegion.right = offset + dwNumEntries;
        destRegion.top = 0;
        destRegion.bottom = 1;
        destRegion.front = 0;
        destRegion.back = 1;
        pD3DDevContext->CopySubresourceRegion(pTex, 0, offset, 0, 0, pTex_Staging, 0, &destRegion);
    };
protected:
private:
    DWORD trans_offset;

    D3D11_MAP lockType;
    bool isLocked;
};



class BASE_DISPLAY_SURFACE : public BASE_VERTEX_DX, public BASE_TEXTURE_DX, public BASE_POSITION_DX {
public:
    BASE_DISPLAY_SURFACE(float inX, float inY, DWORD inWidth, DWORD inHeight, DWORD in_colour_bits, DWORD in_bgColour, bool isRenderTarget, bool hasStagingTexture) :
        BASE_VERTEX_DX(),
        BASE_TEXTURE_DX(in_bgColour),
        BASE_POSITION_DX(inX, inY, 1, false)
    {
        scale_type = SCALE_TYPE::none;
        scaled_Width = (float)width * scaleX;
        scaled_Height = (float)height * scaleY;
        p_pixel_shader = nullptr;
        p_sampler_state = pd3dPS_SamplerState_Linear;
        DXGI_FORMAT dxgi_Format = DXGI_FORMAT_UNKNOWN;
        if (in_colour_bits == 8) {
            dxgi_Format = DXGI_FORMAT_R8_UNORM;
            p_pixel_shader = pd3d_PS_Basic_Tex_8;
            p_sampler_state = pd3dPS_SamplerState_Point;
        }
        else if (in_colour_bits == 16)
            dxgi_Format = DXGI_FORMAT_B5G5R5A1_UNORM;
        else {
            dxgi_Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            p_pixel_shader = pd3d_PS_Basic_Tex_32;
        }

        SetBaseDimensions(inWidth, inHeight);
        if (!CreateVerticies())
            Debug_Info_Error("Failed RenderTarget CreateVerticies.");
        if (!Texture_Initialize(dxgi_Format, isRenderTarget, hasStagingTexture))
            Debug_Info_Error("Failed RenderTarget CreateTexture.");
        //SetMatrices();
    };
    ~BASE_DISPLAY_SURFACE() {
    };
    ID3D11SamplerState* Get_Default_SamplerState() { return p_sampler_state; };
    void Set_Default_SamplerState(ID3D11SamplerState* in_p_sampler_state) { p_sampler_state = in_p_sampler_state; };
    void Set_Default_Pixel_Shader(ID3D11PixelShader* in_p_pixel_shader) { p_pixel_shader = in_p_pixel_shader; };
    void Display() { Display(p_pixel_shader); };
    void Display(ID3D11PixelShader* pd3d_PixelShader);

    void ScaleTo(float in_width, float in_height);
    void ScaleTo(float in_width, float in_height, SCALE_TYPE in_scale_type) {
        scale_type = in_scale_type;
        ScaleTo(in_width, in_height);
    };
  
    float GetScaledWidth() const { return (float)width * scaleX; };
    float GetScaledHeight() const { return height * scaleY; };
protected:
    ID3D11PixelShader* p_pixel_shader;
    ID3D11SamplerState* p_sampler_state;
    float scaled_Width;
    float scaled_Height;
    SCALE_TYPE scale_type;
private:
    
};



class RenderTarget : public BASE_DISPLAY_SURFACE {
public:
    RenderTarget(float inX, float inY, DWORD inWidth, DWORD inHeight, DWORD in_colour_bits, DWORD in_bgColour) :
        BASE_DISPLAY_SURFACE(inX, inY, inWidth, inHeight, in_colour_bits, in_bgColour, true, false)
    {
        SetMatrices();
    };
    ~RenderTarget() {
    };
    ID3D11RenderTargetView* GetRenderTargetView() {
        return pTex_renderTargetView;
    }
protected:
private:
};



class DrawSurface : public BASE_DISPLAY_SURFACE {
public:
    DrawSurface(float inX, float inY, DWORD inWidth, DWORD inHeight, DWORD in_colour_bits, DWORD in_bgColour) :
        BASE_DISPLAY_SURFACE(inX, inY, inWidth, inHeight, in_colour_bits, in_bgColour, false, true)
    {
        SetMatrices();
    };
    ~DrawSurface() {
    };
    HRESULT Lock(VOID** lpSurface, LONG* p_lPitch) {
        HRESULT result;
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        result = pD3DDevContext->Map(pTex_Staging, 0, D3D11_MAP_WRITE, 0, &mappedResource);
        if (lpSurface)
            *lpSurface = mappedResource.pData;
        *p_lPitch = mappedResource.RowPitch;
        return result;
    };
    HRESULT Unlock() {
        pD3DDevContext->Unmap(pTex_Staging, 0);
        pD3DDevContext->CopyResource(pTex, pTex_Staging);
        return 0;
    };
    BOOL Clear_Texture(DWORD colour) {
        if (!pTex_Staging || !pTex)
            return FALSE;
        void* pSurface = nullptr;
        LONG pitch = 0;
        if (Lock(&pSurface, &pitch) != S_OK)
            return FALSE;
        if (pixelWidth == 1)
            memset(pSurface, (BYTE)colour, pitch * height);
        else if (pixelWidth == 2) {
            DWORD size = pitch / 2 * height;
            WORD colourOffset = 0;
            while (colourOffset < size) {
                ((WORD*)pSurface)[colourOffset] = (WORD)colour;
                colourOffset++;
            }
        }
        else if (pixelWidth == 4) {
            DWORD size = pitch / 4 * height;
            DWORD colourOffset = 0;
            while (colourOffset < size) {
                ((DWORD*)pSurface)[colourOffset] = colour;
                colourOffset++;
            }
        }
        Unlock();
        return TRUE;
    }
protected:

private:
};


class DrawSurface8 : public DrawSurface {
public:
    DrawSurface8(DWORD inWidth, DWORD inHeight, bool has_mask_colour, BYTE pal_mask_offset) :
        DrawSurface(0, 0, inWidth, inHeight, 8, 0)
    {
        if (has_mask_colour)
            p_pixel_shader = pd3d_PS_Basic_Tex_8_masked;
        mask_colour = { pal_mask_offset / 255.0f,0.0f,0.0f,0.0f };
        SetMatrices();
    };
    ~DrawSurface8() {
    };
    void Display_Masked() { 
        Shader_SetPaletteData(mask_colour); 
        Display(p_pixel_shader);
    };
protected:
    void SetMatrices() {
        MATRIX_DATA posData{};
        DirectX::XMMATRIX Ortho2D;
        GetOrthoMatrix(&Ortho2D);
        posData.World = DirectX::XMMatrixTranslation(0, 0, (float)0.0f);
        posData.WorldViewProjection = XMMatrixMultiply(posData.World, Ortho2D);
        UpdatePositionData(0, &posData);
    };
private:
    DirectX::XMFLOAT4 mask_colour;
};



class DrawSurface8_RT : public RenderTarget {
public:
    DrawSurface8_RT(float inX, float inY, DWORD inWidth, DWORD inHeight, DWORD in_colour_bits, DWORD in_bgColour, bool has_mask, BYTE pal_mask_offset) :
        RenderTarget(inX, inY, inWidth, inHeight, in_colour_bits, in_bgColour)
    {
        
        staging = new DrawSurface8(width, height, has_mask, pal_mask_offset);
    };
    ~DrawSurface8_RT() {
        delete staging;
    };

    HRESULT Lock(VOID** lpSurface, LONG* p_lPitch) {
        return staging->Lock(lpSurface, p_lPitch);
    }
    HRESULT Unlock() {
        HRESULT result = staging->Unlock();

        ID3D11DepthStencilView* p_depthStencilView = GetDepthStencilView();
        ClearRenderTarget(p_depthStencilView);
        SetRenderTarget(p_depthStencilView);


        staging->Display_Masked();

        return result;
    };
    BOOL Clear_Texture(DWORD colour) {
        staging->Clear_Texture(colour);
        ClearRenderTarget(GetDepthStencilView());
        return TRUE;
    };
protected:
private:
    DrawSurface8* staging;
   
};


extern UINT clientWidth;
extern UINT clientHeight;

extern DrawSurface8_RT* surface_gui;
extern DrawSurface* surface_space3D;
extern DrawSurface8_RT* surface_space2D;
extern DrawSurface8_RT* surface_movieXAN;

extern BOOL space_view_has_BG_image;

extern BOOL crop_cockpit_rect;
extern SCALE_TYPE cockpit_scale_type;
extern BOOL is_nav_view;


void Palette_Update(BYTE* p_pal_buff, BYTE offset, DWORD num_entries);

DrawSurface8_RT* Get_Space2D_Surface();

BOOL Display_Dx_Setup(HWND hwnd, UINT width, UINT height);
void Display_Dx_Destroy();
BOOL Display_Dx_Resize(UINT width, UINT height);
void Display_Dx_Present(PRESENT_TYPE present_type);
void Display_Dx_Present();

BOOL Get_Monitor_Refresh_Rate(HWND hwnd, DXGI_RATIONAL* refreshRate);

void Inflight_Mono_Colour_Setup(DWORD colour, UINT brightness, UINT contrast);
void Reset_DX11_Shader_Defaults();

void Load_Cockpit_HD_Background(const char* cockpit_name);
//Get the cockpit HD Background surface for the current camera view.
DrawSurface* Get_Cockpit_HD_BG_Surface();
//Get the cockpit HD Background surface for this POV, Cockpit, CockLeft, CockRight or CockBack.
DrawSurface* Get_Cockpit_HD_BG_Surface(WORD view_type);
void Destroy_Cockpit_HD_Background();
void Set_Space2D_Surface_SamplerState_From_Config();
void Set_Space2D_Surface_SamplerState_Point();

void Set_Gamma_Offset(UINT gamma);
