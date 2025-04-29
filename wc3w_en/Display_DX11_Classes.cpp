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


//_________________________________________________________________
void BASE_DISPLAY_SURFACE::ScaleTo(float in_width, float in_height) {
    scaled_Width = (float)width;
    scaled_Height = (float)height;

    if (scale_type == SCALE_TYPE::fit) {
        float rtRO = width / (float)height;
        float winRO = in_width / (float)in_height;

        if (rtRO > winRO) {
            x = 0;
            scaled_Width = (float)in_width;
            scaled_Height = (scaled_Width / rtRO);
            y = ((float)in_height - scaled_Height) / 2;
        }
        else {
            y = 0;
            scaled_Height = (float)in_height;
            scaled_Width = scaled_Height * rtRO;
            x = ((float)in_width - scaled_Width) / 2;
        }

        scaleX = scaled_Width / width;
        scaleY = scaled_Height / height;
    }
    else if (scale_type == SCALE_TYPE::fit_best) {
        //Keep the display width and height a multiple of the texture width and height, in order to maintain original pixel alignment.
        float max_height = (float)((DWORD)in_height / height * height);
        float max_width = (float)((DWORD)in_width / width * width);

        float rtRO = width / (float)height;
        float winRO = in_width / (float)in_height;

        if (rtRO > winRO) {
            scaled_Width = max_width;
            scaled_Height = (scaled_Width / rtRO);
        }
        else {
            scaled_Height = max_height;
            scaled_Width = scaled_Height * rtRO;
        }
        x = ((float)in_width - scaled_Width) / 2;
        y = ((float)in_height - scaled_Height) / 2;

        scaleX = scaled_Width / width;
        scaleY = scaled_Height / height;
    }
    else if (scale_type == SCALE_TYPE::fill) {
        x = 0;
        y = 0;
        scaleX = in_width / (float)width;
        scaleY = in_height / (float)height;
        scaled_Width = (float)in_width;
        scaled_Height = (float)in_height;
    }

    SetMatrices();
}


//_____________________________________________________________________
void BASE_DISPLAY_SURFACE::Display(ID3D11PixelShader* pd3d_PixelShader) {

    if (pTex_shaderResourceView == nullptr)
        return;

    UINT stride = sizeof(VERTEX_BASE);
    UINT offset = 0;
    //set vertex stuff
    pD3DDevContext->IASetVertexBuffers(0, 1, &pd3dVB, &stride, &offset);
    //set texture stuff
    pD3DDevContext->PSSetShaderResources(0, 1, &pTex_shaderResourceView);
    //set shader constant buffers for positon and scaling.
    SetPositionRender(0);
    //set pixel shader stuff
    pD3DDevContext->PSSetShader(pd3d_PixelShader, nullptr, 0);
    //set sampling state
    pD3DDevContext->PSSetSamplers(0, 1, &p_sampler_state);

    pD3DDevContext->DrawIndexed(4, 0, 0);
}


//___________________________
void STORED_VIEW::StoreView() {

    pD3DDevContext->OMGetRenderTargets(1, &pRenderTargetViews, &pDepthStencilView);

    pD3DDevContext->RSGetViewports(&numViewPorts, nullptr);
    if (numViewPorts > 0) {
        if (pCViewPorts)
            delete[] pCViewPorts;

        pCViewPorts = new D3D11_VIEWPORT[numViewPorts];
        pD3DDevContext->RSGetViewports(&numViewPorts, pCViewPorts);

        if (pScissorRects)
            delete[] pScissorRects;

        pScissorRects = new D3D11_RECT[numViewPorts];
        pD3DDevContext->RSGetScissorRects(&numViewPorts, pScissorRects);
    }
}


//_____________________________
void STORED_VIEW::ReStoreView() {

    pD3DDevContext->OMSetRenderTargets(1, &pRenderTargetViews, pDepthStencilView);

    if (pCViewPorts != nullptr) {
        pD3DDevContext->RSSetViewports(numViewPorts, pCViewPorts);
    }
    if (pScissorRects != nullptr) {
        pD3DDevContext->RSSetScissorRects(numViewPorts, pScissorRects);
    }
}
