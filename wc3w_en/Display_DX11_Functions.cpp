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

#include "pch.h"

#include "Display_DX11.h"

#include "modifications.h"
#include "memwrite.h"
#include "configTools.h"
#include "movies_vlclib.h"
#include "wc3w.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

using namespace std;

//vertex shaders
#include "shaders\compiled_h\VS_Basic.h"

//pixel shaders
#include "shaders\compiled_h\PS_Basic_Tex_32.h"
#include "shaders\compiled_h\PS_Basic_Tex_8.h"
#include "shaders\compiled_h\PS_Basic_Tex_8_masked.h"
#include "shaders\compiled_h\PS_Greyscale_Tex_32.h"





//Direct3D device and swap chain.
ID3D11Device* g_d3dDevice = nullptr;
ID3D11Device2* g_d3dDevice2 = nullptr;
ID3D11DeviceContext* g_d3dDeviceContext = nullptr;
ID3D11DeviceContext2* g_d3dDeviceContext2 = nullptr;
IDXGISwapChain* g_d3dSwapChain = nullptr;
IDXGISwapChain1* g_d3dSwapChain1 = nullptr;

//Render target view for the back buffer of the swap chain.
ID3D11RenderTargetView* g_d3dRenderTargetView = nullptr;
//Depth/stencil view for use as a depth buffer.
ID3D11DepthStencilView* g_d3dDepthStencilView = nullptr;
//A texture to associate to the depth stencil view.
ID3D11Texture2D* g_d3dDepthStencilBuffer = nullptr;

//Define the functionality of the depth/stencil stages.
ID3D11DepthStencilState* g_d3dDepthStencilState = nullptr;
//Define the functionality of the rasterizer stage.
ID3D11RasterizerState* g_d3dRasterizerState = nullptr;
D3D11_VIEWPORT g_Viewport = { 0 };



//max texture dimensions defined by driver
UINT max_texDim = 8192;


// Shader data
ID3D11VertexShader* pd3dVertexShader_Main = nullptr;
ID3D11InputLayout* pd3dVS_InputLayout_Main = nullptr;

ID3D11PixelShader* pd3d_PS_Basic_Tex_32 = nullptr;
ID3D11PixelShader* pd3d_PS_Basic_Tex_8 = nullptr;
ID3D11PixelShader* pd3d_PS_Basic_Tex_8_masked = nullptr;
ID3D11PixelShader* pd3d_PS_Greyscale_Tex_32 = nullptr;


ID3D11SamplerState* pd3dPS_SamplerState_Point = nullptr;
ID3D11SamplerState* pd3dPS_SamplerState_Linear = nullptr;

ID3D11BlendState* pBlendState_Zero = nullptr;
ID3D11BlendState* pBlendState_One = nullptr;
ID3D11BlendState* pBlendState_Two = nullptr;
ID3D11BlendState* pBlendState_Three = nullptr;
ID3D11BlendState* pBlendState_Four = nullptr;

//every texture is drawn to a basic quad so uses the same index buffer;
ID3D11Buffer* pVB_Quad_IndexBuffer = nullptr;

ID3D11Buffer* pVB_Quad_Line_IndexBuffer = nullptr;



BUFFER_DX* palette_buff_data = nullptr;
XMFLOAT4 pal_mask_movie_text = { 0.0f,0.0f,0.0f,0.0f };
XMFLOAT4 pal_mask_cockpit_hud = { 1.0f,0.0f,0.0f,0.0f };

BUFFER_DX* inflight_colour_options_buff_data = nullptr;
COLOUR_BUFF_DATA inflight_colour_options_buff;
//XMFLOAT4 inflight_colour = { 1.0f,0.0f,0.0f,0.0f };
//XMFLOAT4 inflight_options = { 1.0f,1.0f,0.0f,0.0f };


PAL_DX* main_pal = nullptr;
DrawSurface8_RT* surface_gui = nullptr;
DrawSurface* surface_space3D = nullptr;
DrawSurface8_RT* surface_space2D = nullptr;
DrawSurface8_RT* surface_movieXAN = nullptr;

RenderTarget* rt_display = nullptr;
RenderTarget* rt_Movie = nullptr;


//___________________________________________
void Shader_SetPaletteData(XMFLOAT4 pal_data) {
    if (palette_buff_data)
        palette_buff_data->UpdateData(g_d3dDeviceContext, 0, &pal_data);
}


//___________________________________________________________________________
void Inflight_Mono_Colour_Setup(DWORD colour, UINT brightness, UINT contrast) {
    if (inflight_colour_options_buff_data == nullptr) {
        inflight_colour_options_buff_data = new BUFFER_DX(1, true, sizeof(COLOUR_BUFF_DATA));
        inflight_colour_options_buff_data->SetForRenderPS(g_d3dDeviceContext, 0, 1);
    }
    float a = ((colour & 0xFF000000) >> 24) / 255.0f;
    float r = ((colour & 0x00FF0000) >> 16) / 255.0f;
    float g = ((colour & 0x0000FF00) >> 8) / 255.0f;
    float b = ((colour & 0x000000FF)) / 255.0f;
    inflight_colour_options_buff.colour_val = { r,g,b  ,1.0f };
    inflight_colour_options_buff.colour_opt = { (float)brightness / 100.0f, (float)contrast/100.0f,0,0};
    inflight_colour_options_buff_data->UpdateData(g_d3dDeviceContext, 0, &inflight_colour_options_buff);
    Debug_Info("Inflight_Mono_Colour_Setup Done b:%f, c:%f r:%f:g%f:b%f", inflight_colour_options_buff.colour_opt.x, inflight_colour_options_buff.colour_opt.y, inflight_colour_options_buff.colour_val.x, inflight_colour_options_buff.colour_val.y, inflight_colour_options_buff.colour_val.z);
}


//________________________________________
static void Inflight_Mono_Colour_Destroy() {
    if (inflight_colour_options_buff_data)
        delete inflight_colour_options_buff_data;
    inflight_colour_options_buff_data = nullptr;
    Debug_Info("Inflight_Mono_Colour_Destroy Done");
}


//_________________________
static void Palette_Setup() {
    if (main_pal == nullptr)
        main_pal = new PAL_DX(256, true);

    if (main_pal) {
        ID3D11ShaderResourceView* palTex = main_pal->GetShaderResourceView();
        g_d3dDeviceContext->PSSetShaderResources(1, 1, &palTex);
    }
    palette_buff_data = new BUFFER_DX(1, true, sizeof(PALETTE_BUFF_DATA));
    palette_buff_data->SetForRenderPS(g_d3dDeviceContext, 0, 0);
    Debug_Info("Palette_Setup Done");
}


//___________________________
static void Palette_Destroy() {
    if (main_pal)
        delete main_pal;
    main_pal = nullptr;

    if (palette_buff_data)
        delete palette_buff_data;
    palette_buff_data = nullptr;
    Debug_Info("Palette_Destroy Done");
}


//___________________________________________________________________
void Palette_Update(BYTE* p_pal_buff, WORD offset, DWORD num_entries) {

    if (main_pal)
        main_pal->SetPalEntries(p_pal_buff, offset, num_entries);
}


//_________________________________________________
static void Surfaces_Setup(UINT width, UINT height) {

    if (surface_gui == nullptr) {
        surface_gui = new DrawSurface8_RT(0, 0, GUI_WIDTH, GUI_HEIGHT, 32, 0x00000000, true, 0);
        surface_gui->ScaleTo((float)width, (float)height, SCALE_TYPE::fit);
        if (!ConfigReadInt(L"MAIN", L"ENABLE_LINEAR_UPSCALING_GUI", CONFIG_MAIN_ENABLE_LINEAR_UPSCALING_GUI))
            surface_gui->Set_Default_SamplerState(pd3dPS_SamplerState_Point);
    }
    if (surface_space2D == nullptr) {
        surface_space2D = new DrawSurface8_RT(0, 0, GUI_WIDTH, GUI_HEIGHT, 32, 0x00000000, true, 255);
        surface_space2D->ScaleTo((float)width, (float)height, SCALE_TYPE::fill);
        if (!ConfigReadInt(L"MAIN", L"ENABLE_LINEAR_UPSCALING_COCKPIT_HUD", CONFIG_MAIN_ENABLE_LINEAR_UPSCALING_COCKPIT_HUD))
            surface_space2D->Set_Default_SamplerState(pd3dPS_SamplerState_Point);
    }
    if (surface_space3D == nullptr) {
        surface_space3D = new DrawSurface(0, 0, width, height, 8, 0x00000000);
    }
    Debug_Info("Surfaces_Setup Done");
}


//__________________________________________________
static void Surfaces_Resize(UINT width, UINT height) {

    if (surface_gui)
        surface_gui->ScaleTo((float)width, (float)height);

    if (surface_movieXAN)
        surface_movieXAN->ScaleTo((float)width, (float)height);

    if (surface_space2D)
        surface_space2D->ScaleTo((float)width, (float)height);

    if (surface_space3D)
        delete surface_space3D;
    surface_space3D = new DrawSurface(0, 0, width, height, 8, 0x00000000);

    Debug_Info("Surfaces_Resize Done - space w:%d, h:%d", surface_space3D->GetWidth(), surface_space3D->GetHeight());
}


//____________________________
static void Surfaces_Destroy() {

    if (surface_gui == nullptr)
        delete surface_gui;
    surface_gui = nullptr;

    if (surface_movieXAN == nullptr)
        delete surface_movieXAN;
    surface_movieXAN = nullptr;

    if (surface_space2D == nullptr)
        delete surface_space2D;
    surface_space2D = nullptr;

    if (surface_space3D == nullptr)
        delete surface_space3D;
    surface_space3D = nullptr;

    Debug_Info("Surfaces_Destroy Done");
}


//____________________________
void MovieRT_SetRenderTarget() {

    if (!rt_Movie)
        rt_Movie = new RenderTarget(0, 0, clientWidth, clientHeight, 32, 0x00000000);

    rt_Movie->ClearRenderTarget(g_d3dDepthStencilView);
    rt_Movie->SetRenderTarget(g_d3dDepthStencilView);
}

//__________________
void MovieRT_Clear() {

    if (!rt_Movie)
        return;
    rt_Movie->ClearRenderTarget(g_d3dDepthStencilView);
}


//_________________________________
static void RenderTargets_Destroy() {

    if (rt_display)
        delete rt_display;
    rt_display = nullptr;
    if (rt_Movie)
        delete rt_Movie;
    rt_Movie = nullptr;
}


//____________________________________
DrawSurface8_RT* Get_Space2D_Surface() {
    return surface_space2D;
}


//________________________________________________
void Display_Dx_Present(PRESENT_TYPE present_type) {

    if (!g_d3dDeviceContext)
        return;

    static PRESENT_TYPE last_present_type = present_type;

    float colour[4]{ 0.0f,0.0f,0.0f,0.0f };
    g_d3dDeviceContext->ClearRenderTargetView(g_d3dRenderTargetView, colour);
    g_d3dDeviceContext->ClearDepthStencilView(g_d3dDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    if (!rt_display)
        rt_display = new RenderTarget(0, 0, clientWidth, clientHeight, 32, 0x00000000);

    rt_display->ClearRenderTarget(g_d3dDepthStencilView);
    rt_display->SetRenderTarget(g_d3dDepthStencilView);

    Set_ViewPort(clientWidth, clientHeight);

    if (present_type == PRESENT_TYPE::space) {
        if (is_nav_view || (is_cockpit_view && cockpit_scale_type == SCALE_TYPE::fit && crop_cockpit_rect)) {//when nav screen is up or the cockpit is visible but not streched to fill the screen, clip 3d space view to the cockpit's rect.
            float x_unit = 0;
            float y_unit = 0;
            float x = 0;
            float y = 0;
            surface_space2D->GetPosition(&x, &y);
            surface_space2D->GetScaledPixelDimensions(&x_unit, &y_unit);
            D3D11_RECT rect{ (LONG)x,(LONG)y, (LONG)x + (LONG)(surface_space2D->GetWidth() * x_unit), (LONG)y + (LONG)(surface_space2D->GetHeight() * y_unit) };
            g_d3dDeviceContext->RSSetScissorRects(1, &rect);
        }
        if (surface_space3D)
            surface_space3D->Display();

        if (pMovie_vlc_Inflight && (*p_wc3_space_view_type == SPACE_VIEW_TYPE::Cockpit || *p_wc3_space_view_type == SPACE_VIEW_TYPE::CockHud) && !is_POV3_view)
            pMovie_vlc_Inflight->Display();

        if (surface_space2D) {
            surface_space2D->Display();
        }
    }
    else {
        if (present_type == PRESENT_TYPE::movie) {
            if (rt_Movie)
                rt_Movie->Display();
        }
        if (surface_gui) {
            surface_gui->Display();
        }
    }

    g_d3dDeviceContext->OMSetRenderTargets(1, &g_d3dRenderTargetView, g_d3dDepthStencilView);
    g_d3dDeviceContext->OMSetDepthStencilState(g_d3dDepthStencilState, 0);

    rt_display->Display(pd3d_PS_Basic_Tex_32);

    HRESULT hr = g_d3dSwapChain->Present(1, 0);
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            //To-Do HandleDeviceLost();
            Debug_Info_Error("Present - DeviceLost.");
        }
        else
            Debug_Info_Error("Present Failed.");
    }
}



/*

//_____________________________________________________________________________________________________
HRESULT SaveTextureToFile(ID3D11Texture2D* pTex_Source, const char* pfile_path, bool isFalloutDataPath) {
    HRESULT hresult;
    ID3D11Device* pD3DDev = GetD3dDevice();
    ID3D11DeviceContext* pD3DDevContext = GetD3dDeviceContext();

    if (!pTex_Source)
        return -1;
    D3D11_TEXTURE2D_DESC desc{ 0 };
    D3D11_RESOURCE_DIMENSION resType = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pTex_Source->GetType(&resType);
    if (resType != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    pTex_Source->GetDesc(&desc);

    desc.BindFlags = 0;
    desc.MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;

    ID3D11Texture2D* pTex_Dest;
    hresult = pD3DDev->CreateTexture2D(&desc, nullptr, &pTex_Dest);
    if (FAILED(hresult))
        return hresult;

    pD3DDevContext->CopyResource(pTex_Dest, pTex_Source);


    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT result;
    result = pD3DDevContext->Map(pTex_Dest, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(result)) {
        Debug_Info_Error("SaveTextureToFile - Failed to Map screenshot texture.");
        return hresult;
    }

    WORD pixelSizeBits = 32;

    if (isFalloutDataPath) {//using fallouts data path and file functions
        if (!SaveBMP_DATA(pfile_path, desc.Width, desc.Height, pixelSizeBits, mappedResource.RowPitch, (BYTE*)mappedResource.pData, nullptr, 0, 0, true))// SaveScreenShot(desc.Width, desc.Height, pixelSizeBits, mappedResource.RowPitch, (BYTE*)mappedResource.pData))
            hresult = E_FAIL;
    }
    else {//using local path and regular c file functions
        if (!SaveBMP(pfile_path, desc.Width, desc.Height, pixelSizeBits, mappedResource.RowPitch, (BYTE*)mappedResource.pData, nullptr, 0, 0, true))// SaveScreenShot(desc.Width, desc.Height, pixelSizeBits, mappedResource.RowPitch, (BYTE*)mappedResource.pData))
            hresult = E_FAIL;
    }
    pD3DDevContext->Unmap(pTex_Dest, 0);

    return hresult;
}




//____________________________________________________
HRESULT HandleScreenshot(ID3D11Texture2D* pTex_Source) {
    HRESULT hresult;
    ID3D11Device* pD3DDev = GetD3dDevice();
    ID3D11DeviceContext* pD3DDevContext = GetD3dDeviceContext();

    if (!pTex_Source)
        return -1;
    D3D11_TEXTURE2D_DESC desc{ 0 };
    D3D11_RESOURCE_DIMENSION resType = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pTex_Source->GetType(&resType);
    if (resType != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    pTex_Source->GetDesc(&desc);

    desc.BindFlags = 0;
    desc.MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;

    ID3D11Texture2D* pTex_Dest;
    hresult = pD3DDev->CreateTexture2D(&desc, nullptr, &pTex_Dest);
    if (FAILED(hresult))
        return hresult;

    pD3DDevContext->CopyResource(pTex_Dest, pTex_Source);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT result;
    result = pD3DDevContext->Map(pTex_Dest, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(result)) {
        Debug_Info_Error("HandleScreenshot - Failed to Map screenshot texture.");
        return hresult;
    }

    WORD pixelSizeBits = 32;

    if (!SaveScreenShot(desc.Width, desc.Height, pixelSizeBits, mappedResource.RowPitch, (BYTE*)mappedResource.pData))
        hresult = E_FAIL;
    pD3DDevContext->Unmap(pTex_Dest, 0);

    return hresult;
}

*/


ID3D11Device* GetD3dDevice() {
    return g_d3dDevice;
}
ID3D11DeviceContext* GetD3dDeviceContext() {
    return g_d3dDeviceContext;
}

ID3D11RenderTargetView* GetRenderTargetView() {
    return g_d3dRenderTargetView;
}
ID3D11DepthStencilView* GetDepthStencilView() {
    return g_d3dDepthStencilView;
}
ID3D11DepthStencilState* GetDepthStencilState() {
    return g_d3dDepthStencilState;
}


//________________________________________
void Set_ViewPort(long width, long height) {
    D3D11_VIEWPORT Viewport{};
    Viewport.Width = static_cast<float>(width);
    Viewport.Height = static_cast<float>(height);
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    g_d3dDeviceContext->RSSetViewports(1, &Viewport);

    D3D11_RECT rect{ 0,0,width ,height };
    g_d3dDeviceContext->RSSetScissorRects(1, &rect);
}


//____________________________________________________________
void Set_ViewPort(float x, float y, float width, float height) {
    D3D11_VIEWPORT Viewport{};
    Viewport.Width = width;
    Viewport.Height = height;
    Viewport.TopLeftX = x;
    Viewport.TopLeftY = y;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    g_d3dDeviceContext->RSSetViewports(1, &Viewport);

    D3D11_RECT rect{ (LONG)x,(LONG)y, (LONG)x + (LONG)width , (LONG)y + (LONG)height };
    g_d3dDeviceContext->RSSetScissorRects(1, &rect);
}


//______________________
bool Shader_Main_Setup() {

    HRESULT hr = S_OK;

    if (!pd3dVertexShader_Main) {
        hr = g_d3dDevice->CreateVertexShader(pVS_Basic_mem, sizeof(pVS_Basic_mem), nullptr, &pd3dVertexShader_Main);
        if (FAILED(hr)) {
            Debug_Info_Error("CreateVertexShader Failed.");
            return false;
        }
    }
    if (!pd3dVS_InputLayout_Main) {
        // Create the input layout for the vertex shader.
        D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] = {
            { "POSITION", 0,  DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        hr = g_d3dDevice->CreateInputLayout(vertexLayoutDesc, _countof(vertexLayoutDesc), pVS_Basic_mem, sizeof(pVS_Basic_mem), &pd3dVS_InputLayout_Main);
        if (FAILED(hr)) {
            Debug_Info_Error("CreateInputLayout Failed.");
            return false;
        }
    }
    if (!pVB_Quad_IndexBuffer) {
        WORD Indicies[6]{
        0,// Top left.
        1,// Bottom left.
        2,// Bottom right.
        3,// Top right.
        0,// Top left.
        2 };// Bottom right.
        
        D3D11_SUBRESOURCE_DATA InitData;
        ZeroMemory(&InitData, sizeof(D3D11_SUBRESOURCE_DATA));

        //Create and initialize the index buffer.
        D3D11_BUFFER_DESC indexBufferDesc;
        ZeroMemory(&indexBufferDesc, sizeof(D3D11_BUFFER_DESC));

        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.ByteWidth = sizeof(WORD) * _countof(Indicies);
        indexBufferDesc.CPUAccessFlags = 0;
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        InitData.pSysMem = Indicies;

        HRESULT hr = g_d3dDevice->CreateBuffer(&indexBufferDesc, &InitData, &pVB_Quad_IndexBuffer);
        if (FAILED(hr)) {
            return false;
        }
    }

    if (!pVB_Quad_Line_IndexBuffer) {
        WORD Indicies[5] {
        0,// Bottom left.
        1,// Top left. 
        2,// Top right.
        3,// Bottom Right.
        4 };// Bottom left.
        
        D3D11_SUBRESOURCE_DATA InitData;
        ZeroMemory(&InitData, sizeof(D3D11_SUBRESOURCE_DATA));

        //Create and initialize the index buffer.
        D3D11_BUFFER_DESC indexBufferDesc;
        ZeroMemory(&indexBufferDesc, sizeof(D3D11_BUFFER_DESC));

        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.ByteWidth = sizeof(WORD) * _countof(Indicies);
        indexBufferDesc.CPUAccessFlags = 0;
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        InitData.pSysMem = Indicies;

        HRESULT hr = g_d3dDevice->CreateBuffer(&indexBufferDesc, &InitData, &pVB_Quad_Line_IndexBuffer);
        if (FAILED(hr)) {
            return false;
        }
    }
    //set vetex shader stuff
    g_d3dDeviceContext->VSSetShader(pd3dVertexShader_Main, nullptr, 0);
    g_d3dDeviceContext->IASetInputLayout(pd3dVS_InputLayout_Main);
    //set index buffer stuff
    g_d3dDeviceContext->IASetIndexBuffer(pVB_Quad_IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    g_d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    if (!pd3d_PS_Basic_Tex_32) {
        hr = g_d3dDevice->CreatePixelShader(pPS_Basic_Tex_32_mem, sizeof(pPS_Basic_Tex_32_mem), nullptr, &pd3d_PS_Basic_Tex_32);
        if (FAILED(hr))
            Debug_Info_Error("CreatePixelShader Failed - pd3d_PS_Basic_Tex_32.");
    }
    if (!pd3d_PS_Basic_Tex_8) {
        hr = g_d3dDevice->CreatePixelShader(pPS_Basic_Tex_8_mem, sizeof(pPS_Basic_Tex_8_mem), nullptr, &pd3d_PS_Basic_Tex_8);
        if (FAILED(hr))
            Debug_Info_Error("CreatePixelShader Failed - pd3d_PS_Basic_Tex_8.");
    }
    if (!pd3d_PS_Basic_Tex_8_masked) {
        hr = g_d3dDevice->CreatePixelShader(pPS_Basic_Tex_8_masked_mem, sizeof(pPS_Basic_Tex_8_masked_mem), nullptr, &pd3d_PS_Basic_Tex_8_masked);
        if (FAILED(hr))
            Debug_Info_Error("CreatePixelShader Failed - pd3d_PS_Basic_Tex_8_masked.");
    }
    if (!pd3d_PS_Greyscale_Tex_32) {
        hr = g_d3dDevice->CreatePixelShader(pPS_Greyscale_Tex_32_mem, sizeof(pPS_Greyscale_Tex_32_mem), nullptr, &pd3d_PS_Greyscale_Tex_32);
        if (FAILED(hr))
            Debug_Info_Error("CreatePixelShader Failed - pd3d_PS_Greyscale.");
    }

    //Create sampler states for texture sampling in the pixel shader.
    if (!pd3dPS_SamplerState_Point || !pd3dPS_SamplerState_Linear) {
        D3D11_SAMPLER_DESC samplerDesc;
        ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));

        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.BorderColor[0] = 1.0f;
        samplerDesc.BorderColor[1] = 1.0f;
        samplerDesc.BorderColor[2] = 1.0f;
        samplerDesc.BorderColor[3] = 1.0f;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        if (!pd3dPS_SamplerState_Point) {
            hr = g_d3dDevice->CreateSamplerState(&samplerDesc, &pd3dPS_SamplerState_Point);
            if (FAILED(hr)) {
                Debug_Info_Error("CreateSamplerState Point Failed: %X", hr);
                return false;
            }
        }
        if (!pd3dPS_SamplerState_Linear) {
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            hr = g_d3dDevice->CreateSamplerState(&samplerDesc, &pd3dPS_SamplerState_Linear);
            if (FAILED(hr)) {
                Debug_Info_Error("CreateSamplerState Linear Failed: %X", hr);
                return false;
            }
        }
    }
    g_d3dDeviceContext->PSSetSamplers(0, 1, &pd3dPS_SamplerState_Point);

    //Create blend states.
    if (!pBlendState_Zero || !pBlendState_One || !pBlendState_Two || !pBlendState_Three || !pBlendState_Four) {
        D3D11_BLEND_DESC blendStateDesc;
        ZeroMemory(&blendStateDesc, sizeof(D3D11_BLEND_DESC));
        blendStateDesc.AlphaToCoverageEnable = FALSE;
        blendStateDesc.IndependentBlendEnable = FALSE;
        blendStateDesc.RenderTarget[0].BlendEnable = FALSE;
        blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        if (!pBlendState_Zero)
            hr = g_d3dDevice->CreateBlendState(&blendStateDesc, &pBlendState_Zero);
        blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
        if (!pBlendState_One)
            hr = g_d3dDevice->CreateBlendState(&blendStateDesc, &pBlendState_One);
        if (!pBlendState_Two) {
            blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_MAX;
            hr = g_d3dDevice->CreateBlendState(&blendStateDesc, &pBlendState_Two);
        }
        if (!pBlendState_Three) {
            //blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
            blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            hr = g_d3dDevice->CreateBlendState(&blendStateDesc, &pBlendState_Three);
        }
        if (!pBlendState_Four) {
            blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
            blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            hr = g_d3dDevice->CreateBlendState(&blendStateDesc, &pBlendState_Four);
        }
        g_d3dDeviceContext->OMSetBlendState(pBlendState_One, nullptr, -1);
    }

    if (hr != S_OK)
        return false;
    return true;
}


//________________________
void Shader_Main_Destroy() {
    if (pd3dVertexShader_Main)
        pd3dVertexShader_Main->Release();
    pd3dVertexShader_Main = nullptr;

    if (pd3dVS_InputLayout_Main)
        pd3dVS_InputLayout_Main->Release();
    pd3dVS_InputLayout_Main = nullptr;

    if (pd3d_PS_Basic_Tex_32)
        pd3d_PS_Basic_Tex_32->Release();
    pd3d_PS_Basic_Tex_32 = nullptr;

    if (pd3d_PS_Basic_Tex_8)
        pd3d_PS_Basic_Tex_8->Release();
    pd3d_PS_Basic_Tex_8 = nullptr;

    if (pd3d_PS_Basic_Tex_8_masked)
        pd3d_PS_Basic_Tex_8_masked->Release();
    pd3d_PS_Basic_Tex_8_masked = nullptr;

    if (pd3d_PS_Greyscale_Tex_32)
        pd3d_PS_Greyscale_Tex_32->Release();
    pd3d_PS_Greyscale_Tex_32 = nullptr;

    if (pd3dPS_SamplerState_Point)
        pd3dPS_SamplerState_Point->Release();
    pd3dPS_SamplerState_Point = nullptr;
    if (pd3dPS_SamplerState_Linear)
        pd3dPS_SamplerState_Linear->Release();
    pd3dPS_SamplerState_Linear = nullptr;

    if (pBlendState_Zero)
        pBlendState_Zero->Release();
    pBlendState_Zero = nullptr;
    if (pBlendState_One)
        pBlendState_One->Release();
    pBlendState_One = nullptr;
    if (pBlendState_Two)
        pBlendState_Two->Release();
    pBlendState_Two = nullptr;
    if (pBlendState_Three)
        pBlendState_Three->Release();
    pBlendState_Three = nullptr;
    if (pBlendState_Four)
        pBlendState_Four->Release();
    pBlendState_Four = nullptr;


    if (pVB_Quad_IndexBuffer)
        pVB_Quad_IndexBuffer->Release();
    pVB_Quad_IndexBuffer = nullptr;


    if (pVB_Quad_Line_IndexBuffer)
        pVB_Quad_Line_IndexBuffer->Release();
    pVB_Quad_Line_IndexBuffer = nullptr;

    Inflight_Mono_Colour_Destroy();
}



//_____________________
void Display_Dx_Destroy() {
    Palette_Destroy();
    Surfaces_Destroy();
    RenderTargets_Destroy();

    if (g_d3dDeviceContext)
        g_d3dDeviceContext->ClearState();

    if (g_d3dDepthStencilView != nullptr) {
        g_d3dDepthStencilView->Release();
        g_d3dDepthStencilView = nullptr;
    }
    if (g_d3dRenderTargetView != nullptr) {
        g_d3dRenderTargetView->Release();
        g_d3dRenderTargetView = nullptr;
    }
    if (g_d3dDepthStencilBuffer != nullptr) {
        g_d3dDepthStencilBuffer->Release();
        g_d3dDepthStencilBuffer = nullptr;
    }
    if (g_d3dDepthStencilState != nullptr) {
        g_d3dDepthStencilState->Release();
        g_d3dDepthStencilState = nullptr;
    }
    if (g_d3dRasterizerState != nullptr) {
        g_d3dRasterizerState->Release();
        g_d3dRasterizerState = nullptr;
    }
    if (g_d3dSwapChain1 != nullptr) {
        g_d3dSwapChain1->Release();
        g_d3dSwapChain1 = nullptr;
    }
    if (g_d3dSwapChain != nullptr) {
        g_d3dSwapChain->Release();
        g_d3dSwapChain = nullptr;
    }
    if (g_d3dDeviceContext2 != nullptr) {
        g_d3dDeviceContext2->Release();
        g_d3dDeviceContext2 = nullptr;
    }
    if (g_d3dDeviceContext != nullptr) {
        g_d3dDeviceContext->Release();
        g_d3dDeviceContext = nullptr;
    }
    if (g_d3dDevice2 != nullptr) {
        g_d3dDevice2->Release();
        g_d3dDevice2 = nullptr;
    }
    if (g_d3dDevice != nullptr) {
        g_d3dDevice->Release();
        g_d3dDevice = nullptr;
    }

    Shader_Main_Destroy();

    Debug_Info("Display_Dx_Destroy Done");
}



//______________________________________________________________________________________________________
//Finds the refresh rate for the primary video card and monitor using the choosen screen width and height.
//This function utilizes code taken from a tutorial at http://www.rastertek.com/, http://www.rastertek.com/dx11s2tut03.html
static bool GetRefreshRate(DXGI_FORMAT format, UINT screenWidth, UINT screenHeight, DXGI_RATIONAL* refreshRate) {
    IDXGIFactory* factory = nullptr;
    IDXGIAdapter* adapter = nullptr;
    IDXGIOutput* adapterOutput = nullptr;

    //Create a DirectX graphics interface factory.
    HRESULT result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
    if (FAILED(result))
        return false;


    //Use the factory to create an adapter for the primary graphics interface (video card).
    result = factory->EnumAdapters(0, &adapter);
    if (FAILED(result))
        return false;

    //Enumerate the primary adapter output (monitor).
    result = adapter->EnumOutputs(0, &adapterOutput);
    if (FAILED(result))
        return false;

    UINT numModes = 0;
    //Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
    result = adapterOutput->GetDisplayModeList(format, DXGI_ENUM_MODES_INTERLACED, &numModes, nullptr);
    if (FAILED(result))
        return false;

    //Create a list to hold all the possible display modes for this monitor/video card combination.
    DXGI_MODE_DESC* displayModeList = new DXGI_MODE_DESC[numModes];


    // Now fill the display mode list structures.
    result = adapterOutput->GetDisplayModeList(format, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
    if (FAILED(result))
        return false;


    // Now go through all the display modes and find the one that matches the screen width and height.
    // When a match is found store the numerator and denominator of the refresh rate for that monitor.
    bool matchFound = false;
    for (UINT i = 0; i < numModes; i++) {
        if (displayModeList[i].Width == screenWidth && displayModeList[i].Height == screenHeight) {
            refreshRate->Numerator = displayModeList[i].RefreshRate.Numerator;
            refreshRate->Denominator = displayModeList[i].RefreshRate.Denominator;
            matchFound = true;
        }
    }


     // Release the display mode list.
    delete[] displayModeList;
    displayModeList = nullptr;

    // Release the adapter output.
    adapterOutput->Release();
    adapterOutput = nullptr;

    // Release the adapter.
    adapter->Release();
    adapter = nullptr;

    // Release the factory.
    factory->Release();
    factory = nullptr;

    return matchFound;
}


//__________________________________________________________________
BOOL Get_Monitor_Refresh_Rate(HWND hwnd, DXGI_RATIONAL* refreshRate) {
    if (!refreshRate)
        return FALSE;
    HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monInfo;
    ZeroMemory(&monInfo, sizeof(MONITORINFO));
    monInfo.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfoA(hmon, &monInfo)) {
        UINT scrWidth = monInfo.rcMonitor.right - monInfo.rcMonitor.left;
        UINT scrHeight = monInfo.rcMonitor.bottom - monInfo.rcMonitor.top;
        if (!GetRefreshRate(DXGI_FORMAT_B8G8R8A8_UNORM, scrWidth, scrHeight, refreshRate)) {
            refreshRate->Numerator = 60, refreshRate->Denominator = 1;
            return FALSE;
        }
        return TRUE;
    }
    refreshRate->Numerator = 60, refreshRate->Denominator = 1;
    return FALSE;
}

//_______________________________________________________
BOOL Display_Dx_Setup(HWND hwnd, UINT width, UINT height) {
    HRESULT hr = S_OK;
    
   
    //HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    //MONITORINFO monInfo;
    //ZeroMemory(&monInfo, sizeof(MONITORINFO));
   // monInfo.cbSize = sizeof(MONITORINFO);
    //if (!GetMonitorInfoA(hmon, &monInfo))
    //    Debug_Info_Error("DxSetup - Failed to GetMonitorInfoA.");
   //UINT scrWidth = monInfo.rcMonitor.right - monInfo.rcMonitor.left;
    //UINT scrHeight = monInfo.rcMonitor.bottom - monInfo.rcMonitor.top;

    //Creates a device that supports BGRA formats (DXGI_FORMAT_B8G8R8A8_UNORM and DXGI_FORMAT_B8G8R8A8_UNORM_SRGB).
    UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;// | D3D11_CREATE_DEVICE_SINGLETHREADED;

#if defined(_DEBUG)
    // If the project is in a debug build, enable debugging via SDK Layers.
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        //To-Do DON'T FORGET TO TURN THESE BACK ON-------------------------------------------------------------------------------------------------------------------------------------------------------------------
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

    //Create the DX 11 device and context.
    hr = D3D11CreateDevice(
        nullptr, //use the default adapter.
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        deviceFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &g_d3dDevice, //The created device.
        nullptr,
        &g_d3dDeviceContext //The created device context.
    );
    if (FAILED(hr)) {
        Debug_Info_Error("DxSetup - Failed D3D11CreateDevice.");
        return 0;
    }

    //Get the maximum texture size of the supported feature level - large game map render targets are tiled to this limit. 
    D3D_FEATURE_LEVEL currentFeatureLevel = g_d3dDevice->GetFeatureLevel();

    if (currentFeatureLevel == D3D_FEATURE_LEVEL_9_1 || currentFeatureLevel == D3D_FEATURE_LEVEL_9_2)
        max_texDim = 2048;
    else if (currentFeatureLevel == D3D_FEATURE_LEVEL_9_3)
        max_texDim = 4096;
    else if (currentFeatureLevel == D3D_FEATURE_LEVEL_10_1 || currentFeatureLevel == D3D_FEATURE_LEVEL_10_0)
        max_texDim = 8192;
    else max_texDim = 16384;

    //testing
    //max_texDim = 512;


    // Get the DXGI factory from the device.
    IDXGIFactory1* dxgiFactory = nullptr;

    IDXGIDevice* dxgiDevice = nullptr;
    hr = g_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
    if (SUCCEEDED(hr)) {
        IDXGIAdapter* adapter = nullptr;
        hr = dxgiDevice->GetAdapter(&adapter);
        if (SUCCEEDED(hr)) {
            hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
            adapter->Release();
        }
        dxgiDevice->Release();
    }

    if (FAILED(hr)) {
        Debug_Info_Error("DxSetup - Failed Obtain DXGI factory.");
        Display_Dx_Destroy();
        return 0;
    }

    UINT bufferCount = 2;
    //Create the swap chain.
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
    if (dxgiFactory2) {
        //DirectX 11.1 or later
        hr = g_d3dDevice->QueryInterface(__uuidof(ID3D11Device2), reinterpret_cast<void**>(&g_d3dDevice2));
        if (SUCCEEDED(hr))
            (void)g_d3dDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext2), reinterpret_cast<void**>(&g_d3dDeviceContext2));


        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Stereo = false;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = bufferCount;
        sd.Scaling = DXGI_SCALING_NONE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;// DXGI_ALPHA_MODE_IGNORE;
        sd.Flags = 0;
        hr = dxgiFactory2->CreateSwapChainForHwnd(g_d3dDevice, hwnd, &sd, nullptr, nullptr, &g_d3dSwapChain1);
        if (SUCCEEDED(hr))
            hr = g_d3dSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_d3dSwapChain));
        dxgiFactory2->Release();
    }
    else {
        //DirectX 11.0
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = bufferCount;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        //GetRefreshRate(DXGI_FORMAT_R8G8B8A8_UNORM, scrWidth, scrHeight, &sd.BufferDesc.RefreshRate);
        Get_Monitor_Refresh_Rate(hwnd, &sd.BufferDesc.RefreshRate);
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain(g_d3dDevice, &sd, &g_d3dSwapChain);
    }

    //prevent DXGI from resizing the window when pressing Alt+Enter.
    dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);

    dxgiFactory->Release();

    if (FAILED(hr)) {
        Debug_Info_Error("DxSetup - Failed CreateSwapChainForHwnd.");
        Display_Dx_Destroy();
        return 0;
    }

    /* The ID3D11Device must have multithread protection */
    ID3D10Multithread* pMultithread = nullptr;
    hr = g_d3dDevice->QueryInterface(__uuidof(ID3D10Multithread), (void**)&pMultithread);
    if (SUCCEEDED(hr)) {
        pMultithread->SetMultithreadProtected(TRUE);
        pMultithread->Release();
    }



    //Initialize the back buffer of the swap chain and associate it to a render target view.
    ID3D11Texture2D* backBuffer = nullptr;
    hr = g_d3dSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
    if (FAILED(hr)) {
        Debug_Info_Error("DxSetup - Failed GetBuffer.");
        Display_Dx_Destroy();
        return 0;
    }

    hr = g_d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &g_d3dRenderTargetView);
    if (backBuffer != nullptr)
        backBuffer->Release();
    backBuffer = nullptr;
    if (FAILED(hr)) {
        Debug_Info_Error("DxSetup - Failed CreateRenderTargetView.");
        Display_Dx_Destroy();
        return 0;
    }

    //Create the depth buffer for use with the depth/stencil view.
    D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
    ZeroMemory(&depthStencilBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));

    depthStencilBufferDesc.ArraySize = 1;
    depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilBufferDesc.CPUAccessFlags = 0;
    depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilBufferDesc.Width = width;
    depthStencilBufferDesc.Height = height;
    depthStencilBufferDesc.MipLevels = 1;
    depthStencilBufferDesc.SampleDesc.Count = 1;
    depthStencilBufferDesc.SampleDesc.Quality = 0;
    depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    hr = g_d3dDevice->CreateTexture2D(&depthStencilBufferDesc, nullptr, &g_d3dDepthStencilBuffer);
    if (FAILED(hr)) {
        Display_Dx_Destroy();
        return 0;
    }

    hr = g_d3dDevice->CreateDepthStencilView(g_d3dDepthStencilBuffer, nullptr, &g_d3dDepthStencilView);
    if (FAILED(hr)) {
        Display_Dx_Destroy();
        return 0;
    }

    //Setup depth/stencil state.
    D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc;
    ZeroMemory(&depthStencilStateDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

    //Create a depth stencil state for 2D rendering.
    depthStencilStateDesc.DepthEnable = FALSE;
    depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilStateDesc.StencilEnable = TRUE;
    depthStencilStateDesc.StencilReadMask = 0xFF;
    depthStencilStateDesc.StencilWriteMask = 0xFF;
    depthStencilStateDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilStateDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depthStencilStateDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilStateDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    depthStencilStateDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilStateDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    depthStencilStateDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilStateDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    hr = g_d3dDevice->CreateDepthStencilState(&depthStencilStateDesc, &g_d3dDepthStencilState);


    //Setup rasterizer state.
    D3D11_RASTERIZER_DESC RSDesc;
    memset(&RSDesc, 0, sizeof(D3D11_RASTERIZER_DESC));
    RSDesc.FillMode = D3D11_FILL_SOLID;
    RSDesc.CullMode = D3D11_CULL_BACK;
    RSDesc.FrontCounterClockwise = FALSE;
    RSDesc.DepthBias = 0;
    RSDesc.SlopeScaledDepthBias = 0.0f;
    RSDesc.DepthBiasClamp = 0;
    RSDesc.DepthClipEnable = TRUE;
    RSDesc.ScissorEnable = TRUE;
    RSDesc.AntialiasedLineEnable = FALSE;
    RSDesc.MultisampleEnable = FALSE;

    ID3D11RasterizerState* pRState = nullptr;
    hr = g_d3dDevice->CreateRasterizerState(&RSDesc, &pRState);
    if (FAILED(hr)) {
        Debug_Info_Error("DxSetup - Failed CreateRasterizerState.");
        Display_Dx_Destroy();
        return 0;
    }
    g_d3dDeviceContext->RSSetState(pRState);
    pRState->Release();

    Set_ViewPort(width, height);
    SetScreenProjectionMatrix_XM(width, height);

    if (!Shader_Main_Setup()) {
        Debug_Info_Error("DxSetup - Shader_Main_Setup Failed.");
        Display_Dx_Destroy();
        return 0;
    }

    Palette_Setup();
    Surfaces_Setup(width, height);

    Debug_Info("Display_Dx_Setup Done");
    return 1;
}



//_____________________________________________
BOOL Display_Dx_Resize(UINT width, UINT height) {

    if (g_d3dDevice == nullptr) {
        return FALSE;
    }

    if (!g_d3dSwapChain) {
        Debug_Info_Error("Display_Dx_Resize - no g_d3dSwapChain");
        return FALSE;
    }

    g_d3dDeviceContext->OMSetRenderTargets(0, 0, 0);
    //Release all outstanding references to its back buffers.
    g_d3dRenderTargetView->Release();

    HRESULT hr;
    //Preserve the existing number of buffers in the swap chain and format.
    //Use the client windows width and height.
    hr = g_d3dSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    if (hr != S_OK)
        Debug_Info_Error("Display_Dx_Resize - Failed ResizeBuffers");

    //Get buffer and create a render-target-view.
    ID3D11Texture2D* pBuffer = nullptr;
    hr = g_d3dSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
    if (hr != S_OK)
        Debug_Info_Error("Display_Dx_Resize - Failed GetBuffer");

    if (pBuffer != nullptr)
        hr = g_d3dDevice->CreateRenderTargetView(pBuffer, nullptr, &g_d3dRenderTargetView);
    else
        Debug_Info_Error("Display_Dx_Resize - Failed pBuffer");
    if (hr != S_OK)
        Debug_Info_Error("Display_Dx_Resize - Failed CreateRenderTargetView");
    pBuffer->Release();

    //Resize the stencil buffer
    if (g_d3dDepthStencilBuffer != nullptr) {
        g_d3dDepthStencilBuffer->Release();
        g_d3dDepthStencilBuffer = nullptr;
    }
    if (g_d3dDepthStencilView != nullptr) {
        g_d3dDepthStencilView->Release();
        g_d3dDepthStencilView = nullptr;
    }


    //Create the depth buffer for use with the depth/stencil view.
    D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
    ZeroMemory(&depthStencilBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));

    depthStencilBufferDesc.ArraySize = 1;
    depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilBufferDesc.CPUAccessFlags = 0; // No CPU access required.
    depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilBufferDesc.Width = width;
    depthStencilBufferDesc.Height = height;
    depthStencilBufferDesc.MipLevels = 1;
    depthStencilBufferDesc.SampleDesc.Count = 1;
    depthStencilBufferDesc.SampleDesc.Quality = 0;
    depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    hr = g_d3dDevice->CreateTexture2D(&depthStencilBufferDesc, nullptr, &g_d3dDepthStencilBuffer);
    if (FAILED(hr))
        Debug_Info_Error("Display_Dx_Resize - Create DepthStencilBuffer Failed");
    if (g_d3dDepthStencilBuffer != nullptr)
        hr = g_d3dDevice->CreateDepthStencilView(g_d3dDepthStencilBuffer, nullptr, &g_d3dDepthStencilView);
    if (FAILED(hr))
        Debug_Info_Error("Display_Dx_Resize - Create DepthStencilView Failed");

    Set_ViewPort(width, height);
    SetScreenProjectionMatrix_XM(width, height);

    Surfaces_Resize(width, height);
    RenderTargets_Destroy();

    return TRUE;





}




//____________________________________________________________________________________________________
bool CreateQuadVB(ID3D11Device* pD3DDev, unsigned int width, unsigned int height, ID3D11Buffer** lpVB) {
    ID3D11Buffer* pVB = nullptr;

    float left = 0.0f;
    float top = 0.0f;
    float right = (float)width;
    float bottom = (float)height;

    VERTEX_BASE Vertices[4]{};

    Vertices[0].Position = XMFLOAT3(left, bottom, 0.0f);  // bottom left.
    Vertices[0].texUV = XMFLOAT2(0.0f, 1.0f);
    Vertices[0].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);

    Vertices[1].Position = XMFLOAT3(left, top, 0.0f);  // Top left.
    Vertices[1].texUV = XMFLOAT2(0.0f, 0.0f);
    Vertices[1].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);


    Vertices[2].Position = XMFLOAT3(right, bottom, 0.0f);  // bottom right.
    Vertices[2].texUV = XMFLOAT2(1.0f, 1.0f);
    Vertices[2].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);

    Vertices[3].Position = XMFLOAT3(right, top, 0.0f);  // top right.
    Vertices[3].texUV = XMFLOAT2(1.0f, 0.0f);
    Vertices[3].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);

    //Fill in a buffer description.
    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));

    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(VERTEX_BASE) * _countof(Vertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;

    //Fill in the subresource data.
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(D3D11_SUBRESOURCE_DATA));
    InitData.pSysMem = Vertices;

    //Create the vertex buffer.
    HRESULT hr = pD3DDev->CreateBuffer(&bufferDesc, &InitData, &pVB);
    if (FAILED(hr))
        return false;

    *lpVB = pVB;
    pVB = nullptr;
    return true;
}



__declspec(align(16)) XMMATRIX Ortho2D_SCRN_XM;

//__________________________________________________________
bool SetScreenProjectionMatrix_XM(DWORD width, DWORD height) {
    Ortho2D_SCRN_XM = XMMatrixOrthographicOffCenterLH(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1000.0f);
    return true;
};


//______________________________________
XMMATRIX* GetScreenProjectionMatrix_XM() {
    return &Ortho2D_SCRN_XM;
};
