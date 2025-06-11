/**********************************************************************************
// Aluno: Rafael Lucas de Azevedo Nunes
// Descrição:  programa para gerar curvas de Bezier
//
**********************************************************************************/

#include "Curves.h"
#include <string>
#include <sstream>
#include <fstream>
#include "DXUT.h"

// ------------------------------------------------------------------------------



// ------------------------------------------------------------------------------

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

// ------------------------------------------------------------------------------

class Curves : public App
{
private:
    ID3D12RootSignature* rootSignature;
    ID3D12PipelineState* pipelineStateLine; //PSO para desenhar as linhas de suporte e curvas
    ID3D12PipelineState* pipelineStateTriangle; //PSO para desenhar os pontos de controle
    VertexBuffer<Vertex>* curvesBuffer; //vbuffer para curvas
    VertexBuffer<Vertex>* controlPointsBuffer; //vbuffer para pontos de controle
    VertexBuffer<Vertex>* supportLineBuffer; //vbuffer para linhas de suporte
    static const uint MaxSize = 5050;//quantidade máxima para vértices das curvas
    static const uint LineSegs = 100; //quantidade de vértices para cada curva
    Vertex vertices[MaxSize];
    Vertex clicks[4]; //array para guardar os quatro pontos das curvas
    Vertex curve[LineSegs + 1];//array para guardar a curva atual
    Vertex controlPoints[18]; //array para guardar os pontos de controle, cada ponto é feito com 6 vértices
    Vertex supportLine[4]; //array para guardar as linhas de suporte
    uint count = 0; //contador de vértices das curvas
    uint click = 0; //contador de cliques para desenhar as curvas
    uint cpCount = 0; //contador para os pontos de controle


public:
    void Init();
    void Update();
    void Draw(VertexBuffer<Vertex>* buffer, int vertexCount, D3D_PRIMITIVE_TOPOLOGY topology);
    void Display();
    void Save();
    void Load();
    void Finalize();
    void BuildRootSignature();
    void BuildPipelineState();
};
void Curves::Init()
{   
   
 
    // cria vertex buffers
    curvesBuffer = new VertexBuffer<Vertex>(nullptr, MaxSize);
    controlPointsBuffer = new VertexBuffer<Vertex>(nullptr, 18);
    supportLineBuffer = new VertexBuffer<Vertex>(nullptr, 4);
    
    BuildRootSignature();
    BuildPipelineState();

    // ------------------

    graphics->SendToGpu();
}

// ------------------------------------------------------------------------------

void Curves::Update()
{
    // sai com o pressionamento da tecla ESC
    if (input->KeyPress(VK_ESCAPE))
        window->Close();

    //Deletar curvas 
    if (input->KeyPress(VK_DELETE)) {
        
        cpCount = 0;
        click = 0;
        count = 0;

        std::memset(vertices, 0, sizeof(vertices));
        std::memset(clicks, 0, sizeof(clicks));
        std::memset(curve, 0, sizeof(curve));
        std::memset(controlPoints, 0, sizeof(controlPoints));
        std::memset(supportLine, 0, sizeof(supportLine));

        graphics->PrepareGpu(pipelineStateTriangle);
        controlPointsBuffer->Copy(controlPoints, 0);
        curvesBuffer->Copy(vertices, 0);
        supportLineBuffer->Copy(supportLine, 0);
        graphics->SendToGpu();

        
        Display();
    }
    //save na tecla 'S'
    if (input->KeyPress('S')) {
        Save();
    }
    //load na tecla 'L'
    if (input->KeyPress('L')) {
        Load();
    }

    if (count < MaxSize) {
        float cx = float(window->CenterX());
        float cy = float(window->CenterY());
        float mx = float(input->MouseX());
        float my = float(input->MouseY());

        // converte as coordenadas da tela para a faixa -1.0 a 1.0
        // cy e my foram invertidos para levar em consideração que 
        // o eixo y da tela cresce na direção oposta do cartesiano
        float x = (mx - cx) / cx;
        float y = (cy - my) / cy;


        // cria vértices com o botão do mouse
        if (input->KeyPress(VK_LBUTTON))
        {

            float s = 0.01f;

            // apenas 3 pontos de controle na tela por vez
            if (cpCount > 17) {
                cpCount = 0;
            }
            // cada ponto de controle é um quadrado feito com dois triângulos
            controlPoints[cpCount] = { XMFLOAT3(x + s, y + s, 0.0f), XMFLOAT4(Colors::Red) };
            controlPoints[cpCount + 1] = { XMFLOAT3(x + s, y - s, 0.0f), XMFLOAT4(Colors::Red) };
            controlPoints[cpCount + 2] = { XMFLOAT3(x - s, y - s, 0.0f), XMFLOAT4(Colors::Red) };
            controlPoints[cpCount + 3] = { XMFLOAT3(x - s, y - s, 0.0f), XMFLOAT4(Colors::Red) };
            controlPoints[cpCount + 4] = { XMFLOAT3(x - s, y + s, 0.0f), XMFLOAT4(Colors::Red) };
            controlPoints[cpCount + 5] = { XMFLOAT3(x + s, y + s, 0.0f), XMFLOAT4(Colors::Red) };
            cpCount += 6;


            graphics->PrepareGpu(pipelineStateLine);
            controlPointsBuffer->Copy(controlPoints, cpCount);
            graphics->SendToGpu();

            //armazena os pontos para a curva
            clicks[click] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::Yellow) };

            click++;
            //no quarto clique desenha a curva
            if (click == 4) {

                for (int i = 0; i <= LineSegs; i++) {
                    float t = 1.0f / LineSegs * i;
                    x = pow(1.0f - t, 3) * clicks[click - 4].Pos.x
                        + 3 * t * pow(1.0f - t, 2) * clicks[click - 3].Pos.x
                        + 3 * t * t * (1.0f - t) * clicks[click - 2].Pos.x
                        + t * t * t * clicks[click - 1].Pos.x;
                    y = pow(1.0f - t, 3) * clicks[click - 4].Pos.y
                        + 3 * t * pow(1.0f - t, 2) * clicks[click - 3].Pos.y
                        + 3 * t * t * (1.0f - t) * clicks[click - 2].Pos.y
                        + t * t * t * clicks[click - 1].Pos.y;
                    curve[i] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::White) };
                }
                for (int i = 0; i <= LineSegs; i++) {
                    vertices[count] = curve[i];
                    count++;
                }
                //depois da primeira curva o primeiro ponto da próxima curva será o ultimo ponto da curva anterior
                clicks[0] = clicks[3];
                click = 1;
            }

            // copia vértices para o buffer da GPU usando o buffer de Upload
            graphics->PrepareGpu(pipelineStateLine);
            curvesBuffer->Copy(vertices, count);
            graphics->SendToGpu();

            Display();

        }
        //no terceiro clique mostra o preview da curva a ser desenhada
        if (click == 3) {


            for (int i = 0; i <= LineSegs; i++) {
                float t = 1.0f / LineSegs * i;
                float x1 = pow(1.0f - t, 3) * clicks[click - 3].Pos.x
                    + 3 * t * pow(1.0f - t, 2) * clicks[click - 2].Pos.x
                    + 3 * t * t * (1.0f - t) * clicks[click - 1].Pos.x
                    + t * t * t * x;
                float y1 = pow(1.0f - t, 3) * clicks[click - 3].Pos.y
                    + 3 * t * pow(1.0f - t, 2) * clicks[click - 2].Pos.y
                    + 3 * t * t * (1.0f - t) * clicks[click - 1].Pos.y
                    + t * t * t * y;
                curve[i] = { XMFLOAT3(x1, y1, 0.0f), XMFLOAT4(Colors::White) };
            }
            for (int i = 0; i <= LineSegs; i++) {
                vertices[count] = curve[i];
                count++;
            }
            graphics->PrepareGpu(pipelineStateLine);
            curvesBuffer->Copy(vertices, count);
            graphics->SendToGpu();


            Display();
            count -= LineSegs + 1;
        }

        //desenha a linha de suporte
        if (click > 0) {

            if (click == 1) {
                supportLine[0] = clicks[0];
                supportLine[1] = { XMFLOAT3(x,y,0.0f), XMFLOAT4(Colors::Yellow) };
            }
            else if (click == 2) {
                supportLine[0] = clicks[0];
                supportLine[1] = clicks[1];
                supportLine[2] = { XMFLOAT3(x,y,0.0f), XMFLOAT4(Colors::Yellow) };
            }
            else {
                supportLine[0] = clicks[0];
                supportLine[1] = clicks[1];
                supportLine[2] = clicks[2];
                supportLine[3] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::Yellow) };
            }

            graphics->PrepareGpu(pipelineStateLine);
            supportLineBuffer->Copy(supportLine, click + 1);
            graphics->SendToGpu();
            Display();
        }
    }
    
}

// ------------------------------------------------------------------------------

void Curves::Draw(VertexBuffer<Vertex>* buffer, int vertexCount, D3D_PRIMITIVE_TOPOLOGY topology) {
    graphics->CommandList()->IASetVertexBuffers(0, 1, buffer->View());
    graphics->CommandList()->IASetPrimitiveTopology(topology);

    graphics->CommandList()->DrawInstanced(vertexCount, 1, 0, 0);
}

void Curves::Display()
{
    // limpa backbuffer
    graphics->Clear(pipelineStateLine);
    graphics->CommandList()->SetGraphicsRootSignature(rootSignature);

    //pontos de controle
    graphics->CommandList()->SetPipelineState(pipelineStateTriangle);
    Draw(controlPointsBuffer, cpCount, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //linha de suporte
    graphics->CommandList()->SetPipelineState(pipelineStateLine);
    Draw(supportLineBuffer, click+1, D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
        
    // curvas
    Draw(curvesBuffer, count, D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
    

    // apresenta backbuffer
    graphics->Present();    
}

// ------------------------------------------------------------------------------


//salva estado atual do programa em um txt 
void Curves::Save() {
    std::ofstream file("curves_save.txt");
    if (!file.is_open()) {
        OutputDebugStringA("Erro ao abrir o arquivo de save.\n");
        return;
    }

    file << count << " " << click << " " << cpCount << "\n";
    //salva os vértices das curvas
    for (uint i = 0; i < count; ++i) {
        file << vertices[i].Pos.x << " " << vertices[i].Pos.y << " " << vertices[i].Pos.z << " ";
        file << vertices[i].Color.x << " " << vertices[i].Color.y << " " << vertices[i].Color.z << " " << vertices[i].Color.w << "\n";
    }
    //salva os cliques 
    for (uint i = 0; i < click; ++i) {
        file << clicks[i].Pos.x << " " << clicks[i].Pos.y << " " << clicks[i].Pos.z << " ";
        file << clicks[i].Color.x << " " << clicks[i].Color.y << " " << clicks[i].Color.z << " " << clicks[i].Color.w << "\n";
    }
    //salva os pontos de controle
    for (uint i = 0; i < cpCount; ++i) {
        file << controlPoints[i].Pos.x << " " << controlPoints[i].Pos.y << " " << controlPoints[i].Pos.z << " ";
        file << controlPoints[i].Color.x << " " << controlPoints[i].Color.y << " " << controlPoints[i].Color.z << " " << controlPoints[i].Color.w << "\n";
    }

    file.close();
}

//lê o txt do save
void Curves::Load() {
    std::ifstream file("curves_save.txt");
    if (!file.is_open()) {
        OutputDebugStringA("Erro ao abrir o arquivo de save.\n");
        return;
    }

    file >> count >> click >> cpCount;
    //carrega os vértices das curvas
    for (uint i = 0; i < count; ++i) {
        file >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
        file >> vertices[i].Color.x >> vertices[i].Color.y >> vertices[i].Color.z >> vertices[i].Color.w;
    }
    //carrega os cliques 
    for (uint i = 0; i < click; ++i) {
        file >> clicks[i].Pos.x >> clicks[i].Pos.y >> clicks[i].Pos.z;
        file >> clicks[i].Color.x >> clicks[i].Color.y >> clicks[i].Color.z >> clicks[i].Color.w;
    }
    //carrega os pontos de controle
    for (uint i = 0; i < cpCount; ++i) {
        file >> controlPoints[i].Pos.x >> controlPoints[i].Pos.y >> controlPoints[i].Pos.z;
        file >> controlPoints[i].Color.x >> controlPoints[i].Color.y >> controlPoints[i].Color.z >> controlPoints[i].Color.w;
    }

    file.close();

    graphics->PrepareGpu(pipelineStateLine);
    curvesBuffer->Copy(vertices, count);
    supportLineBuffer->Copy(supportLine, click+1);
    controlPointsBuffer->Copy(controlPoints, cpCount);
    
    graphics->SendToGpu();

    Display();
}

void Curves::Finalize()
{
    // espera GPU finalizar comandos pendentes
    graphics->WaitForGpu();

    // libera memória alocada
    rootSignature->Release();
    pipelineStateLine->Release();
    pipelineStateTriangle->Release();
    delete curvesBuffer;
    delete controlPointsBuffer;
    delete supportLineBuffer;
}

void Curves::BuildRootSignature()
{
    // descrição para uma assinatura vazia
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 0;
    rootSigDesc.pParameters = nullptr;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // serializa assinatura raiz
    ID3DBlob* serializedRootSig = nullptr;
    ID3DBlob* error = nullptr;

    ThrowIfFailed(D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig,
        &error));

    // cria uma assinatura raiz vazia
    ThrowIfFailed(graphics->Device()->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)));
}

// ------------------------------------------------------------------------------

void Curves::BuildPipelineState()
{
    // --------------------
    // --- Input Layout ---
    // --------------------

    D3D12_INPUT_ELEMENT_DESC inputLayout[2] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // --------------------
    // ----- Shaders ------
    // --------------------

    ID3DBlob* vertexShader;
    ID3DBlob* pixelShader;

    D3DReadFileToBlob(L"Shaders/Vertex.cso", &vertexShader);
    D3DReadFileToBlob(L"Shaders/Pixel.cso", &pixelShader);

    // --------------------
    // ---- Rasterizer ----
    // --------------------

    D3D12_RASTERIZER_DESC rasterizer = {};
    rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer.FrontCounterClockwise = FALSE;
    rasterizer.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = FALSE;
    rasterizer.AntialiasedLineEnable = FALSE;
    rasterizer.ForcedSampleCount = 0;
    rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // ---------------------
    // --- Color Blender ---
    // ---------------------

    D3D12_BLEND_DESC blender = {};
    blender.AlphaToCoverageEnable = FALSE;
    blender.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
    {
        FALSE,FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        blender.RenderTarget[i] = defaultRenderTargetBlendDesc;

    // ---------------------
    // --- Depth Stencil ---
    // ---------------------

    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = TRUE;
    depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencil.StencilEnable = FALSE;
    depthStencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
    { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
    depthStencil.FrontFace = defaultStencilOp;
    depthStencil.BackFace = defaultStencilOp;

    // -----------------------------------
    // --- Pipeline State Object (PSO) ---
    // -----------------------------------


    //PSO para desenhar as curvas e linhas de suporte
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoLine = {};
    psoLine.pRootSignature = rootSignature;
    psoLine.VS = { reinterpret_cast<BYTE*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
    psoLine.PS = { reinterpret_cast<BYTE*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
    psoLine.BlendState = blender;
    psoLine.SampleMask = UINT_MAX;
    psoLine.RasterizerState = rasterizer;
    psoLine.DepthStencilState = depthStencil;
    psoLine.InputLayout = { inputLayout, 2 };
    psoLine.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    psoLine.NumRenderTargets = 1;
    psoLine.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoLine.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoLine.SampleDesc.Count = graphics->Antialiasing();
    psoLine.SampleDesc.Quality = graphics->Quality();
    graphics->Device()->CreateGraphicsPipelineState(&psoLine, IID_PPV_ARGS(&pipelineStateLine));

    //PSO para desenhar os pontos de controle
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoTriangle = {};
    psoTriangle.pRootSignature = rootSignature;
    psoTriangle.VS = { reinterpret_cast<BYTE*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
    psoTriangle.PS = { reinterpret_cast<BYTE*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
    psoTriangle.BlendState = blender;
    psoTriangle.SampleMask = UINT_MAX;
    psoTriangle.RasterizerState = rasterizer;
    psoTriangle.DepthStencilState = depthStencil;
    psoTriangle.InputLayout = { inputLayout, 2 };
    psoTriangle.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoTriangle.NumRenderTargets = 1;
    psoTriangle.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoTriangle.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoTriangle.SampleDesc.Count = graphics->Antialiasing();
    psoTriangle.SampleDesc.Quality = graphics->Quality();
    graphics->Device()->CreateGraphicsPipelineState(&psoTriangle, IID_PPV_ARGS(&pipelineStateTriangle));
    vertexShader->Release();
    pixelShader->Release();
}

// ------------------------------------------------------------------------------
//                                  WinMain                                      
// ------------------------------------------------------------------------------

int APIENTRY WinMain(_In_ HINSTANCE hInstance,    _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    try
    {
        // cria motor e configura a janela
        Engine* engine = new Engine();
        engine->window->Mode(ASPECTRATIO);
        engine->window->Size(1024, 600);
        engine->window->Color(25, 25, 25);
        engine->window->Title("Curves");
        engine->window->Icon("Icon");
        engine->window->LostFocus(Engine::Pause);
        engine->window->InFocus(Engine::Resume);

        // cria e executa a aplicação
        engine->Start(new Curves());

        // finaliza execução
        delete engine;
    }
    catch (Error & e)
    {
        // exibe mensagem em caso de erro
        MessageBox(nullptr, e.ToString().data(), "Curves", MB_OK);
    }

    return 0;
}

// ----------------------------------------------------------------------------
