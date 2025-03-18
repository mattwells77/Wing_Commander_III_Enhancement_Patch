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


//_________________________
void GEN_SURFACE::Display() {

    if (pD3DDevContext == nullptr)
        return;

    UINT stride = sizeof(VERTEX_BASE);
    UINT offset = 0;
    //set vertex stuff
    pD3DDevContext->IASetVertexBuffers(0, 1, &pd3dVB, &stride, &offset);

    //set texture stuff
    pD3DDevContext->PSSetShaderResources(0, 1, &pTex_shaderResourceView);


    //set shader constant buffers
    SetPositionRender(0);

    ID3D11PixelShader* pd3d_PixelShader = pd3d_PS_Basic_Tex_8;
    if (GetPixelWidth() == 4)
        pd3d_PixelShader = pd3d_PS_Basic_Tex_32;
    //set pixel shader stuff
    pD3DDevContext->PSSetShader(pd3d_PixelShader, nullptr, 0);

    pD3DDevContext->DrawIndexed(4, 0, 0);
}


//____________________________________________________________
void GEN_SURFACE::Display(ID3D11PixelShader* pd3d_PixelShader) {

    if (pTex_shaderResourceView == nullptr)
        return;

    UINT stride = sizeof(VERTEX_BASE);
    UINT offset = 0;
    //set vertex stuff
    pD3DDevContext->IASetVertexBuffers(0, 1, &pd3dVB, &stride, &offset);
    //set texture stuff
    pD3DDevContext->PSSetShaderResources(0, 1, &pTex_shaderResourceView);
    //set shader constant buffers 
    SetPositionRender(0);
    //set pixel shader stuff
    pD3DDevContext->PSSetShader(pd3d_PixelShader, nullptr, 0);

    pD3DDevContext->DrawIndexed(4, 0, 0);

}


//_______________________________
void GEN_SURFACE::ScaleToScreen() {
    display_w = (float)width;
    display_h = (float)height;

    if (current_scale_type == SCALE_TYPE::fit) {
        float rtRO = width / (float)height;
        float winRO = clientWidth / (float)clientHeight;

        if (rtRO > winRO) {
            x = 0;
            display_w = (float)clientWidth;
            display_h = (display_w / rtRO);
            y = ((float)clientHeight - display_h) / 2;
        }
        else {
            y = 0;
            display_h = (float)clientHeight;
            display_w = display_h * rtRO;
            x = ((float)clientWidth - display_w) / 2;
        }

        scaleX = display_w / width;
        scaleY = display_h / height;
    }
    else if (current_scale_type == SCALE_TYPE::fit_best) {
        //Keep the display width and height a multiple of the texture width and height, in order to maintain original pixel alignment.
        float max_height = (float)(clientHeight / height * height);
        float max_width = (float)(clientWidth / width * width);

        float rtRO = width / (float)height;
        float winRO = clientWidth / (float)clientHeight;

        if (rtRO > winRO) {
            display_w = max_width;
            display_h = (display_w / rtRO);
        }
        else {
            display_h = max_height;
            display_w = display_h * rtRO;
        }
        x = ((float)clientWidth - display_w) / 2;
        y = ((float)clientHeight - display_h) / 2;

        scaleX = display_w / width;
        scaleY = display_h / height;
    }
    else if (current_scale_type == SCALE_TYPE::fill) {
        x = 0;
        y = 0;
        scaleX = clientWidth / (float)width;
        scaleY = clientHeight / (float)height;
        display_w = (float)clientWidth;
        display_h = (float)clientHeight;
    }

    SetMatrices();
}


//____________________________________________________
void GEN_SURFACE::ScaleToScreen(SCALE_TYPE scale_type) {

    current_scale_type = scale_type;
    ScaleToScreen();
}


//____________________________
void RenderTarget::Display() {
    if (pTex_shaderResourceView == nullptr)
        return;

    //set vertex stuff
    UINT stride = sizeof(VERTEX_BASE);
    UINT offset = 0;
    pD3DDevContext->IASetVertexBuffers(0, 1, &pd3dVB, &stride, &offset);
    //set texture stuff
    pD3DDevContext->PSSetShaderResources(0, 1, &pTex_shaderResourceView);
    //set shader constant buffers 
    SetPositionRender(0);
    //set pixel shader stuff
    pD3DDevContext->PSSetShader(pd3d_PS_Basic_Tex_32, nullptr, 0);

    pD3DDevContext->DrawIndexed(4, 0, 0);
}


//_____________________________________________________________
void RenderTarget::Display(ID3D11PixelShader* pd3d_PixelShader) {

    if (pTex_shaderResourceView == nullptr)
        return;

    UINT stride = sizeof(VERTEX_BASE);
    UINT offset = 0;
    //set vertex stuff
    pD3DDevContext->IASetVertexBuffers(0, 1, &pd3dVB, &stride, &offset);
    //set texture stuff
    pD3DDevContext->PSSetShaderResources(0, 1, &pTex_shaderResourceView);
    //set shader constant buffers 
    SetPositionRender(0);
    //set pixel shader stuff
    pD3DDevContext->PSSetShader(pd3d_PixelShader, nullptr, 0);

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

/*
//_________________________________________________________
void RenderTarget::ClearRect(XMFLOAT4 colour, RECT* pRect) {
    if (pRect && (pRect->right <= pRect->left || pRect->bottom <= pRect->top))
        return;

    if (!pRect || (pRect->left <= 0 && pRect->top <= 0 && pRect->right >= (LONG)width && pRect->bottom >= (LONG)height)) {
        float colourf[4] = { colour.x, colour.y, colour.z, colour.w };
        ClearRenderTarget(nullptr, colourf);
        return;
    }
    if (pRect->right < 0 || pRect->bottom < 0 || pRect->left >= (LONG)width || pRect->top >= (LONG)height)
        return;


    unique_ptr<STORED_VIEW> store_view(new STORED_VIEW(true));

    SetRenderTargetAndViewPort(nullptr);

    XMMATRIX Ortho2D;
    GetOrthoMatrix(&Ortho2D);
    MATRIX_DATA posData;
    posData.World = DirectX::XMMatrixTranslation((float)0, (float)0, (float)0);
    posData.WorldViewProjection = DirectX::XMMatrixMultiply(posData.World, Ortho2D);
    pPS_BuffersFallout->UpdatePositionBuff(&posData);
    pPS_BuffersFallout->SetPositionRender();
    pD3DDevContext->RSSetScissorRects(1, pRect);


    pD3DDevContext->OMSetBlendState(pBlendState_Zero, nullptr, -1);
    //set vertex stuff
    ID3D11Buffer* pd3dVB;
    GetVertexBuffer(&pd3dVB);
    UINT stride = sizeof(VERTEX_BASE);
    UINT offset = 0;
    pD3DDevContext->IASetVertexBuffers(0, 1, &pd3dVB, &stride, &offset);

    GEN_SURFACE_BUFF_DATA genSurfaceData;
    genSurfaceData.genData4_1 = colour;
    pPS_BuffersFallout->UpdateBaseBuff(&genSurfaceData);
    pD3DDevContext->PSSetShader(pd3d_PS_Colour32, nullptr, 0);

    pD3DDevContext->DrawIndexed(4, 0, 0);

    pD3DDevContext->OMSetBlendState(pBlendState_One, nullptr, -1);
}
*/
