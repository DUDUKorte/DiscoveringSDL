#include "Renderer3D.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include "Utils/LeandroMath.h"

#include "Utils/Log.h"


std::vector<uint32_t>* Renderer3D::Buffer = nullptr;
std::vector<float>* Renderer3D::ZBuffer = nullptr;
SDL_GPUTexture* Renderer3D::DepthTexture = nullptr;

std::vector<MeshLight*>* Renderer3D::SceneLights = nullptr;
glm::vec3 Renderer3D::AmbientLight = { 0.2f, 0.2f, 0.2f };
LightAlgorithms Renderer3D::LightAlgorithm = Constant;

WindowProperties* Renderer3D::MainWindow = nullptr;
uint32_t Renderer3D::DrawColor = LeandroColor(0.8f, 0.8f, 0.2f, 1.0f);
std::vector<std::vector<std::pair<glm::vec3, glm::vec3>>> Renderer3D::intersecoes(0);

SDL_GPUShader* Renderer3D::m_VertexShader = nullptr;
SDL_GPUShader* Renderer3D::m_FragmentShaderFill = nullptr;
SDL_GPUShader* Renderer3D::m_FragmentShaderWireframe = nullptr;

bool Renderer3D::WireframeMode = false;
bool Renderer3D::UsePerspective = false;
SDL_GPUGraphicsPipeline* Renderer3D::DefaultPipeline = nullptr;
SDL_GPUGraphicsPipeline* Renderer3D::WireframePipeline = nullptr;

#ifndef USE_HARDWARE_RENDERER
RendererModes Renderer3D::RendererMode = RendererModes::Software;
#else
RendererModes Renderer3D::RendererMode = RendererModes::Hardware;
#endif

const char* vertexShaderSource = R"(
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragGouraudColor;

struct GPULight {
    vec4 posAndType;
    vec4 colorAndMaxDist;
};

layout(set = 1, binding = 0) uniform VertexUniforms {
    mat4 modelMatrix;
    mat4 viewProjMatrix;
    mat4 normalMatrix;
	mat4 cameraMatrix;
} ubo;

layout(set = 1, binding = 1) uniform LightingUniforms {
    vec4 baseColor;
    vec4 materialProps;
    vec4 ambientLight;
    int lightAlgorithm;
    int numLights;
    int perspective, pad1;
    GPULight lights[16];
} lightUbo;

vec3 EvaluateLighting(vec3 normal, vec3 worldPos) {
    vec3 N = normalize(normal);
    vec3 baseCol = lightUbo.baseColor.rgb;
    vec3 ambient = lightUbo.ambientLight.rgb * lightUbo.materialProps.x * baseCol;
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);
    vec3 V = vec3(0.0, 0.0, -1.0);

    for (int i = 0; i < lightUbo.numLights; i++) {
        vec3 lightPos = lightUbo.lights[i].posAndType.xyz;
        float isPoint = lightUbo.lights[i].posAndType.w;
        vec3 lightIntensity = lightUbo.lights[i].colorAndMaxDist.rgb;
        float maxDist = lightUbo.lights[i].colorAndMaxDist.w;

        float dist = distance(worldPos, lightPos);
        float attenuation = 1.0;

        if (isPoint > 0.5) {
            if (dist > maxDist) continue;
            attenuation = min(1.0 / (dist * dist), 1.0);
        }

        vec3 L = normalize(lightPos - worldPos);
        float diff = max(dot(N, L), 0.0);
        diffuse += diff * lightIntensity * lightUbo.materialProps.y * baseCol * attenuation;

        if (diff > 0.0) {
            vec3 R = reflect(-L, N);
            float spec = pow(max(dot(R, V), 0.0), lightUbo.materialProps.w);
            specular += spec * lightIntensity * lightUbo.materialProps.z * attenuation;
        }
    }
    return clamp(ambient + diffuse + specular, 0.0, 1.0);
}

void main() {
    vec4 worldPos = ubo.modelMatrix * vec4(inPosition, 1.0);
	vec4 telaPos = worldPos;

    fragWorldPos = worldPos.xyz;
    fragNormal = normalize(mat3(ubo.normalMatrix) * inNormal);

    // Se for Gouraud (1), já calcula a cor no vértice para a GPU interpolar
    if (lightUbo.lightAlgorithm == 1) {
        fragGouraudColor = EvaluateLighting(fragNormal, fragWorldPos);
    } else {
        fragGouraudColor = vec3(0.0);
    }

    gl_Position = ubo.viewProjMatrix * telaPos;
}
)";

const char* fragmentShaderSource = R"(
#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragGouraudColor;

layout(location = 0) out vec4 outColor;

struct GPULight {
    vec4 posAndType;
    vec4 colorAndMaxDist;
};

layout(set = 3, binding = 0) uniform LightingUniforms {
    vec4 baseColor;
    vec4 materialProps;
    vec4 ambientLight;
    int lightAlgorithm;
    int numLights;
    int perspective, pad1;
    GPULight lights[16];
} lightUbo;

vec3 EvaluateLighting(vec3 normal, vec3 worldPos) {
    vec3 N = normalize(normal);
    vec3 baseCol = lightUbo.baseColor.rgb;
    vec3 ambient = lightUbo.ambientLight.rgb * lightUbo.materialProps.x * baseCol;
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);
    vec3 V = vec3(0.0, 0.0, -1.0);

    for (int i = 0; i < lightUbo.numLights; i++) {
        vec3 lightPos = lightUbo.lights[i].posAndType.xyz;
        float isPoint = lightUbo.lights[i].posAndType.w;
        vec3 lightIntensity = lightUbo.lights[i].colorAndMaxDist.rgb;
        float maxDist = lightUbo.lights[i].colorAndMaxDist.w;

        float dist = distance(worldPos, lightPos);
        float attenuation = 1.0;

        if (isPoint > 0.5) {
            if (dist > maxDist) continue;
            attenuation = pow(1.0 - (clamp(dist / maxDist, 0.0, 1.0)), 2.0);
			//fatAtenuacao = glm::pow(1.0f - glm::clamp(lightDistance / light->GetMaxDistance(), 0.0f, 1.0f), 2.0f);
        }

        vec3 L = normalize(lightPos - worldPos);
        float diff = max(dot(N, L), 0.0);
        diffuse += diff * lightIntensity * lightUbo.materialProps.y * baseCol * attenuation;

        if (diff > 0.0) {
            vec3 R = reflect(-L, N);
            float spec = pow(max(dot(R, V), 0.0), lightUbo.materialProps.w);
            specular += spec * lightIntensity * lightUbo.materialProps.z * attenuation;
        }
    }
    return clamp(ambient + diffuse + specular, 0.0, 1.0);
}

void main() {
    if (lightUbo.lightAlgorithm == 1) {
        // Gouraud: usa direto a cor interpolada vinda do Vertex Shader
        outColor = vec4(fragGouraudColor, 1.0);
    } 
    else if (lightUbo.lightAlgorithm == 0) {
        // Constant: reconstrói a normal chapada da face usando derivadas de tela
        vec3 flatNormal = normalize(cross(dFdx(fragWorldPos), dFdy(fragWorldPos)));
        if (dot(flatNormal, fragNormal) < 0.0) flatNormal = -flatNormal;
        outColor = vec4(EvaluateLighting(flatNormal, fragWorldPos), 1.0);
    } 
    else {
        // Phong (2): usa a normal interpolada por pixel
        outColor = vec4(EvaluateLighting(fragNormal, fragWorldPos), 1.0);
    }
}
)";


const char* fragmentShaderWireframeSource = R"(
#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragGouraudColor;

layout(location = 0) out vec4 outColor;

struct GPULight {
    vec4 posAndType;
    vec4 colorAndMaxDist;
};

layout(set = 3, binding = 0) uniform LightingUniforms {
    vec4 baseColor;
    vec4 materialProps;
    vec4 ambientLight;
    int lightAlgorithm;
    int numLights;
    int perspective, pad1;
    GPULight lights[16];
} lightUbo;

void main() {
	outColor = vec4(0.0, 0.0, 1.0, 1.0);
}
)";

// Inicializa o renderer de Hardware
// tudo comentado aqui já
void Renderer3D::InitGPU(WindowProperties* window)
{
	Renderer3D::MainWindow = window; // dã

	// Cria o zbuffer q a GPU vai usar aqui
	// aparentemente a gpu faz o zbuffer e td mais automaticamente
	// esse zbuffer varia de 0.0 até 1.0 (1.0 é mais longe)
	// usa o znear e zfar como base pra manter no range 0-1
	// (é literalmente só uma textura, mas agnt manda a gpu usar como o zbuffer e td só funciona)
	SDL_GPUTextureCreateInfo depthInfo = {};
	depthInfo.type = SDL_GPU_TEXTURETYPE_2D; // pq a tela é 2D, lógico
	depthInfo.width = MainWindow->Width; // tem q ser do tamanho da tela
	depthInfo.height = MainWindow->Height; // se n, n funciona né fi
	depthInfo.layer_count_or_depth = 1;
	depthInfo.num_levels = 1;
	depthInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT; // aparentemente esse é o tipo de valor padrão q a gpu usa pro zbuffer (float de 32 bits)
	depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET; // aqui agnt fala q é pra usar a textura como zbuffer (depth stencil é tipo pincel de profundidade aka zbuffer)
	DepthTexture = SDL_CreateGPUTexture(MainWindow->GPUDevice, &depthInfo);
	// vale lembrar q toda vez q a tela muda de tamanho tem q recriar o zbuffer com o novo tamanho e mandar ele pra ser usado pela gpu
	
	// SHader compilation
	// Da uma olhada nos coméntários da própria função do CompileGLSLToSDLShader (tá no .h)
	m_VertexShader = CompileGLSLToSDLShader(MainWindow->GPUDevice, vertexShaderSource, shaderc_glsl_vertex_shader, SDL_GPU_SHADERSTAGE_VERTEX, 2);
	m_FragmentShaderFill = CompileGLSLToSDLShader(MainWindow->GPUDevice, fragmentShaderSource, shaderc_glsl_fragment_shader, SDL_GPU_SHADERSTAGE_FRAGMENT, 1);
	m_FragmentShaderWireframe = CompileGLSLToSDLShader(MainWindow->GPUDevice, fragmentShaderWireframeSource, shaderc_glsl_fragment_shader, SDL_GPU_SHADERSTAGE_FRAGMENT, 1);

	CreateGPUPipeline();
}


// Cria/Recria a pipeline da GPU
// Tirei do init pra poder trocar entre render normal e wireframe
// td comentado aqui já
void Renderer3D::CreateGPUPipeline()
{
	// Layout dos Vértices (GPUVertex: vec3 pos + vec3 normal)
	SDL_GPUVertexBufferDescription vertexBufferDesc = {}; // isso aqui 'ensina' pra gpu como ler o GPUBuffer q mandamos a cada Mesh em UploadMeshToGPU()
	vertexBufferDesc.slot = 0;
	vertexBufferDesc.pitch = sizeof(GPUVertex); // tamanho de cada vértice, é isso aqui q vai pro in do shader
	vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	// Aqui agnt 'ensina' pra gpu como ler cada vértice pq n é só putaria nessa vram nn
	// principalmente pq tem a pos e o normal no vertex né
	SDL_GPUVertexAttribute vertexAttributes[2] = {}; // tamanho 2 pq tem 2 variáveis na struct (dã)
	// position
	vertexAttributes[0].location = 0; // lembra disso pq é como vamo pega o valor lá dentro do shader (location = x)
	vertexAttributes[0].buffer_slot = 0;
	vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; // formato 3 flaots (float, float, float) -> isso mesmo, um vec3 :o
	vertexAttributes[0].offset = offsetof(GPUVertex, position); // offset né (engraçado q no arquivo original tem literalmente um 'if cplusplus: else: ' p separar c de c++ akjlhjdkfkjsdhfj)

	// normal
	vertexAttributes[1].location = 1;
	vertexAttributes[1].buffer_slot = 0;
	vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	vertexAttributes[1].offset = offsetof(GPUVertex, normal);

	//Aí se quiser adicionar mais atributos é aqui pra baixo né, aí vai colocando vertexAttributes[i+1] (NÃO ESQUECE O LOCATION) (NEM O OFFSET)

	// Target de Cor e Depth Stencil
	SDL_GPUColorTargetDescription colorTargetDesc = {};
	colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(MainWindow->GPUDevice, MainWindow->Window);

	// Aqui começa a criar o pipeline pra GPU usar
	//Me veio um pensamento agora: pra desenhar o outline agnt faz OUTRA pipeline? ou tenta modificar essa? pode isso?
	//TODO Testar modificar/criar a pipeline em runtime com o Mesh->GetOutline(); // talvez verificar o outline ANTES de começar o renderpass?
	// e aí sim recriar a pipeline? n sei, tem q testar dps
	SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.vertex_shader = m_VertexShader;
	pipelineInfo.fragment_shader = m_FragmentShaderFill;
	pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST; // manda ela usar triângulos pra desenhar, então obrigatoriamente todas as meshes precisam estar trianguladas
	// se n ele vai comer mais/menos vértices mandando pro vertexShader e o desenho simplesmente quebra (ou n desenha)

	pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &vertexBufferDesc;
	pipelineInfo.vertex_input_state.num_vertex_buffers = 1; // acho difícil agnt ter mais de 1 vertex buffer
	pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;
	pipelineInfo.vertex_input_state.num_vertex_attributes = 2; // Lembra de mudar aqui tbm se for adicioanr mais atributos no vertexShader!!!!!!

	pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL; // Preenche os triângulos, na teoria seria só mudar isso aqui pra fazer o outline (SDL_GPU_FILLMODE_LINE)
	pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK; // Backface CUlling meio q 'automático' com o zbuffer q vamos passar

	// Z-Buffer ==> agnt 'binda' o DepthBuffer a cada frame lá no update and render quando começa o RenderPass
	pipelineInfo.depth_stencil_state.enable_depth_test = true; // usar/ler zbuffer = sim
	pipelineInfo.depth_stencil_state.enable_depth_write = true; // escrever/salvar zbuffer = tbm
	pipelineInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL; // Z atual <= Z lido = sobrescrever o Z lido

	pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
	pipelineInfo.target_info.num_color_targets = 1;
	pipelineInfo.target_info.has_depth_stencil_target = true;
	pipelineInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

	Renderer3D::DefaultPipeline = SDL_CreateGPUGraphicsPipeline(MainWindow->GPUDevice, &pipelineInfo);
	// Cria a pipeline de wireframe
	pipelineInfo.fragment_shader = m_FragmentShaderWireframe;
	pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
	pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
	Renderer3D::WireframePipeline = SDL_CreateGPUGraphicsPipeline(MainWindow->GPUDevice, &pipelineInfo);
	// TODO: Cria a pipeline do outline

	// Depois de criar o pipeline, o SDL_GPU permite liberar os objetos de shader
	// Decidi não liberar os shaders pra usar posteriormente quando atualiza entre fill e wireframe
	//SDL_ReleaseGPUShader(MainWindow->GPUDevice, vertShader);
	//SDL_ReleaseGPUShader(MainWindow->GPUDevice, fragShader);
}

// Faz upload dos vértices da mesh pra GPU
// PRECISA fazer isso pra funcionar o renderer Hardware
// Tudo comentado aqui já
void Renderer3D::UploadMeshToGPU(Mesh& mesh)
{
	// Não tem mesh, pode ser só um objeto de lógica
	if (mesh.GetObj() == "None") { return; }

	std::vector<GPUVertex> gpuVertices;

	auto& localVertices = mesh.GetLocalVertices();
	auto& vertexNormals = mesh.GetVertexNormals();
	auto& faceNormals = mesh.GetNormals();

	if (mesh.GetFaces()[0].vertices.size() > 3) { return; } // Mesh inválida (não triangulada)

	// tem que popular o gpu vertices com o vertice e normal da mehs
	for (const face& f : mesh.GetFaces()) {
		if (f.vertices.size() < 3) continue;

		for (size_t i = 0; i < 3; i++) {
			int vIdx = f.vertices[i];
			glm::vec3 pos = localVertices[vIdx];
			glm::vec3 norm = (vIdx < localVertices.size()) ? vertexNormals[vIdx] : faceNormals[f.normal];
			gpuVertices.push_back({ pos, norm });
		}
	}

	if (gpuVertices.empty()) { return; }
	mesh.GetVertexCount() = static_cast<uint32_t>(gpuVertices.size());
	uint32_t byteSize = mesh.GetVertexCount() * sizeof(GPUVertex);

	// criar vertex buffer (buffer que fica na VRAM da gpu, de onde vai puxar a mesh)
	SDL_GPUBufferCreateInfo bufInfo = {};
	bufInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	bufInfo.size = byteSize;
	mesh.SetGPUBuffer(SDL_CreateGPUBuffer(MainWindow->GPUDevice, &bufInfo));

	// cria transfer buffer
	// aqui é tipo um buffer q fica em uma memória compartilahda entre a cpu e gpu
	SDL_GPUTransferBufferCreateInfo transInfo = {};
	transInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	transInfo.size = byteSize;
	SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(MainWindow->GPUDevice, &transInfo);
	// esse é um ponteiro pra memória compartilhada
	void* mapPtr = SDL_MapGPUTransferBuffer(MainWindow->GPUDevice, transferBuffer, false);
	memcpy(mapPtr, gpuVertices.data(), byteSize); // aqui todas as vértices são copiadas pra essa memória compartilogada
	SDL_UnmapGPUTransferBuffer(MainWindow->GPUDevice, transferBuffer); // aqui o ponteiro p essa memória é fechado
	// só q ainda tá 'jogado' na memória compartilhada, no transfer buffer, n tá dentro da vram (GPU BUffer)

	// copy pass pra gpu : copia os conteúdos q ainda tão no transfer buffer pro GPUBuffer (Vram) de fato
	// aí a gpu vai saber q aqls dados são os vértices da mesh
	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(MainWindow->GPUDevice); // usa o command buffer pq vai fazer isso tudo só no submit
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
	// origem (transferbuffer)
	SDL_GPUTransferBufferLocation srcLoc;
	srcLoc.transfer_buffer = transferBuffer;
	srcLoc.offset = 0;
	//destino o GPU Buffer mesmo
	SDL_GPUBufferRegion dstReg;
	dstReg.buffer = mesh.GetGPUBuffer();
	dstReg.offset = 0;
	dstReg.size = byteSize;

	SDL_UploadToGPUBuffer(copyPass, &srcLoc, &dstReg, false); // aqui sim 'manda' p vram (escreve no commandbuffer na vdd)
	SDL_EndGPUCopyPass(copyPass); 
	SDL_SubmitGPUCommandBuffer(commandBuffer); // AQUI SIM ele roda os comando q agnt fez (basicamente só o copypass)

	SDL_ReleaseGPUTransferBuffer(MainWindow->GPUDevice, transferBuffer); // libera a memória compartilhada pq n vai mais usar
}

// é
void Renderer3D::DrawMeshGPU(Mesh& mesh, SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass)
{
	// só em via das dúvidas
	if (!mesh.GetGPUBuffer())
	{
		UploadMeshToGPU(mesh); // Talvez colocar um limite? tentar fazer o upload 3 vezes e marcar a mesh como invalid pra parar de tentar, se n fica spamando erro no cmd
		if (!mesh.GetGPUBuffer())
		{
			LE_CORE_WARN("AVISO: Mesh incompativel com Hardware Renderer!!! Verifique se seu modelo 3D esta triangularizado!!!!");
			return;
		}
	}

	if (Renderer3D::WireframeMode) {
		SDL_BindGPUGraphicsPipeline(renderPass, WireframePipeline);
	}

	// mapeando coordenadas de tela
	VertexUniformData vertUbo = {};
	vertUbo.modelMatrix = mesh.GetTransformMatrix(); // mat4 com as transformações da mesh
	vertUbo.normalMatrix = glm::transpose(glm::inverse(vertUbo.modelMatrix)); // conserta os normais se tiver escalonado não uniformemente

	// projeção ortográfica ou perspectiva, aplica a transformação da câmera também (gambiarra)
	glm::mat4 projMatrix;
	glm::mat4 viewMatrix(1.0f);
	// matriz da câmera p aplicar
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), MainWindow->CameraPos);
	transform = glm::rotate(transform, glm::radians(MainWindow->CameraRot.y), { 0.0f, 1.0f, 0.0f });
	transform = glm::rotate(transform, glm::radians(MainWindow->CameraRot.x), { 1.0f, 0.0f, 0.0f });

	if (Renderer3D::UsePerspective) { // aplica perpectiva (usando a criação de matriz de perspectiva)
		projMatrix = glm::perspectiveLH_ZO(glm::radians(60.0f), static_cast<float>(MainWindow->Width)/MainWindow->Height, 0.1f, 100000.0f);
		projMatrix[1][1] *= -1.0f; // arruma o y (n sei pq tá invertido, talvez arrumar pra usar y = 0 ==> topo da tela)
	}
	else { // não aplica perspectiva
		projMatrix = glm::ortho(
			0.0f, static_cast<float>(MainWindow->Width),
			static_cast<float>(MainWindow->Height), 0.0f,
			10000.0f, -10000.0f
		);
	}

	viewMatrix = glm::inverse(transform);
	vertUbo.cameraMatrix = glm::mat4(1.0f); // nem tá usando isso aqui na vdd, poderia remover

	vertUbo.viewProjMatrix = projMatrix * viewMatrix;

	// dados de material
	MeshMaterial& mat = mesh.GetMaterial();
	FragmentUniformData fragUbo = {};
	fragUbo.baseColor = glm::vec4(mat.baseColor, 1.0f);
	fragUbo.materialProps = glm::vec4(mat.ambientReflection, mat.diffuse, mat.specular, mat.specular_brightness);
	fragUbo.ambientLight = glm::vec4(AmbientLight, 1.0f);
	fragUbo.lightAlgorithm = static_cast<int>(Renderer3D::LightAlgorithm);

	if (SceneLights && mesh.GetAfectedByLights()) {
		int count = std::min(static_cast<int>(SceneLights->size()), MAX_LIGHTS);
		fragUbo.numLights = count;

		for (int i = 0; i < count; i++) {
			MeshLight* light = (*SceneLights)[i];

			float isPoint = (light->GetLightType() != LightType::DirectionalLight) ? 1.0f : 0.0f;			
			fragUbo.lights[i].posAndType = glm::vec4(light->GetTransform().Position, isPoint);
			fragUbo.lights[i].colorAndMaxDist= glm::vec4(light->GetLightColor() * light->GetLightIntensity(), light->GetMaxDistance());
		}
	}else {
		// caso não for afetado por luz
		fragUbo.numLights = 0;
		fragUbo.ambientLight = glm::vec4(1.0f);
		fragUbo.materialProps = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		fragUbo.lightAlgorithm = 2;
	}

	// binda os valores em vertex shader e fragment shader
	SDL_PushGPUVertexUniformData(commandBuffer, 0, &vertUbo, sizeof(VertexUniformData));
	SDL_PushGPUVertexUniformData(commandBuffer, 1, &fragUbo, sizeof(FragmentUniformData));
	SDL_PushGPUFragmentUniformData(commandBuffer, 0, &fragUbo, sizeof(FragmentUniformData));

	// faz o Bind do vertex buffer da mesh e desenha
	SDL_GPUBufferBinding binding = {};
	binding.buffer = mesh.GetGPUBuffer(); // aql buffer lá q tem todas os GPUVertex da mesh
	binding.offset = 0;
	// faz o bind né:	    (renderPass,slot, binding , qtd);
	SDL_BindGPUVertexBuffers(renderPass, 0  , &binding, 1);
	// aqui manda o comando pra desenhar
	// tem que informar a quantia de vértices, quantas instâncias desenhar, o primeiro vértice (provavelmente nunca vai ser != 0) e a primeira instância
	// Talvez se quiser fazer um objeto q é único mas por algm motivo se repete dezenas de vezes? (partículas???)
	SDL_DrawGPUPrimitives(renderPass, mesh.GetVertexCount(), 1, 0, 0);
	// Desenha cada submesh, porém tem algum problema com a herança do transform nas submeshes
	// e não faço ideia do porque elas tão com problema... (será q é porque ele atualiza a matriz de transformação, mas )
	for (BaseMesh* sm : mesh.GetSubMeshes()) {
		if (sm->GetType() == MeshTypes::Type3D) {
			Mesh& smc = *(static_cast<Mesh*>(sm));
			DrawMeshGPU(smc, commandBuffer, renderPass);
		}
	}

	if (mesh.GetOutline()) {
		// Aqui iria fazer o bind do pipeline de outline e desenhar de novo a mesh
		//SDL_BindGPUGraphicsPipeline(renderPass, WireframePipeline);
		// DrawMeshGPU(mesh, commandBuffer, renderPass);
	}
}

void Renderer3D::Init(std::vector<uint32_t>* buffer, std::vector<float>* zbuffer, WindowProperties* window, std::vector<MeshLight*>* lights)
{
	Buffer = buffer;
	ZBuffer = zbuffer;
	SceneLights = lights;
	MainWindow = window;
	Renderer3D::intersecoes = std::vector<std::vector<std::pair<glm::vec3, glm::vec3>>>(MainWindow->Height);
}

// Atualiza/Redimensiona o vetor de intersecoes (Software)
// e o ZBuffer da GPU (Hardware)
// o ZBuffer do software é redimensionado na função de WINDOW_RESIZE
// do próprio Editor pq as informações do sotware estão lá mesmo
// (tem q mudar isso depois inclusive...).
// tudo comentado aqui já
void Renderer3D::UpdateResize() {
	// Tem que redimensionar as intersecoes também, não fiz um if pra separar entre software e hardware
	// pq pode acontecer da pessoa aumentar a janela em hardware e trocar pra software
	// aí crasha td pq o intersecoes teria q ser reatualizado quando troca de renderere e etc... mt chato, só 
	// deixa atualizar tudo logo de uma vez q n faz mal p ngm... (menos p consumo de ram adlskfhjhksjdfhlakjs)
	Renderer3D::intersecoes = std::vector<std::vector<std::pair<glm::vec3, glm::vec3>>>(MainWindow->Height);

	// OBLITERA O ZBUFFER DA FACE DA TERRA kjahshdkjasdghfjkswdrhfajklshlfgkashjklfhs djklfhasdlk
	SDL_ReleaseGPUTexture(MainWindow->GPUDevice, DepthTexture);

	// Recria o zbuffer se n fica tudo preto e fudeu o renderer
	// n vou re-comentar tudo q é isso aqui -> olha lá no Renderer3D::Init()
	SDL_GPUTextureCreateInfo depthInfo = {};
	depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
	depthInfo.width = MainWindow->Width;
	depthInfo.height = MainWindow->Height;
	depthInfo.layer_count_or_depth = 1;
	depthInfo.num_levels = 1;
	depthInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
	depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
	DepthTexture = SDL_CreateGPUTexture(MainWindow->GPUDevice, &depthInfo);
}

void Renderer3D::DrawLine(glm::vec3 source, glm::vec3 target)
{
	const int width = MainWindow->Width;
	const int height = MainWindow->Height;

	float dX = (target.x - source.x);
	float dY = (target.y - source.y);

	if (!std::isfinite(dX) || !std::isfinite(dY)) { return; }

	if ((source.x < 0 && target.x < 0) || (source.x >= width && target.x >= width) ||
		(source.y < 0 && target.y < 0) || (source.y >= height && target.y >= height)) {
		return;
	}

	float steps = static_cast<float>((int)std::ceil(std::max(std::abs(dX), std::abs(dY))));

	if (steps == 0) {
		int x = static_cast<int>(std::lround(source.x));
		int y = static_cast<int>(std::lround(source.y));
		if (x >= 0 && x < width && y >= 0 && y < height) {
			Buffer->at(y * MainWindow->Width + x) = DrawColor;
		}
		return;
	}

	float xInc = dX / steps;
	float yInc = dY / steps;
	float zInc = (target.z - source.z) / dX;

	float new_x = source.x;
	float new_y = source.y;

	float currentZ = source.z;
	int xStart = static_cast<int>(std::ceil(source.x));
	currentZ += (static_cast<float>(xStart) - source.x) * (dX > 0.0001f ? zInc : 1.0f);
	
	for (int i = 0; i <= steps; i++) {   // <= inclui o ponto final
		int px = static_cast<int>(std::lround(new_x));
		int py = static_cast<int>(std::lround(new_y));

		if (px >= 0 && px < width && py >= 0 && py < height) {
			if (currentZ+2000.0f < ZBuffer->at(py * MainWindow->Width + px)) {
				Buffer->at(py * MainWindow->Width + px) = DrawColor;
				ZBuffer->at(py * MainWindow->Width + px) = currentZ;
			}
		}

		new_x += xInc;
		new_y += yInc;
		currentZ += (dX > 0.0001f ? zInc : 0.0f);
	}

	//int ymin = (int)source.y;
	//int ymax = (int)target.y;
	//for (int y = ymin; y < ymax; y++) {
	//	vertexBuffer->at(y * 1280 + int(new_x)) = LeandroColor(0.2f, 0.2f, 0.8, 1.0);
	//	new_x += Tx;
	//}
}

void Renderer3D::Draw3DLinesWithVertices(const std::vector<std::pair<glm::vec3, glm::vec3>>& vertices)
{
	if (vertices.size() < 2) { // só desenha se tiver 2 ou mais vertices
		//Buffer->at(vertices[0].first.y * MainWindow->Width + vertices[0].first.x) = DrawColor;
		//SDL_RenderPoint(renderer, vertices[0].x, vertices[0].y);
		return;
	}

	for (int i = 0; i < vertices.size() - 1; i++) {

		DrawLine(vertices[i].first, vertices[i + 1].first);
		//SDL_RenderLine(renderer, vertices[i].x, vertices[i].y, vertices[i + 1].x, vertices[i + 1].y);

		if (vertices.size() - 2 == i) {
			//SDL_RenderLine(renderer, vertices[i + 1].x, vertices[i + 1].y, vertices[0].x, vertices[0].y);
			DrawLine(vertices[i + 1].first, vertices[0].first);
		}
	}
}

void Renderer3D::DrawMesh(Mesh& meshTeste)
{
	if (Renderer3D::RendererMode == RendererModes::Hardware) { return; }

	std::vector<glm::vec3>& allVertices = meshTeste.GetVertices();
	std::vector<glm::vec3>& allLocalVertices = meshTeste.GetLocalVertices();
	std::vector<glm::vec3>& meshNormals = meshTeste.GetNormals();
	std::vector<glm::vec3> verticesNormals = {};
	std::vector<std::pair<glm::vec3, glm::vec3>> currentVertices = {}; // ==> {vertice(x,y,z) ; cor(r,g,b)}
	glm::vec2 minMaxFace = { 10000, -1 };

	// Luzes pra testar a iluminação
	// TODO: Criar objetos de luz pra modificar na cena

	for (face& currentFace : meshTeste.GetFaces()) {
		// Backface culling
		glm::vec4 meshNormal4 = { meshNormals[currentFace.normal].x, meshNormals[currentFace.normal].y, meshNormals[currentFace.normal].z, 0.0f };
		meshNormal4 = meshTeste.GetTransformMatrix() * meshNormal4;
		glm::vec3 meshNormal = { meshNormal4.x, meshNormal4.y, meshNormal4.z };
		meshNormal = glm::normalize(meshNormal);

		if (glm::dot(meshNormal, { 0.0f, 0.0f, 1.0f }) >= 0) {
			continue;
		}

		bool interpolateVertices = false;
		minMaxFace = { 10000.0f, -1.0f };
		currentVertices.clear();
		glm::vec3 verticesSum = { 0.0f, 0.0f, 0.0f };

		for (int i : currentFace.vertices) {
			glm::vec3 verticeTela = allVertices[i];

			//// aplica a câmera (ou tenta)
			verticeTela = verticeTela - MainWindow->CameraPos;

			// a rotação tá estranha, mas serve por enquanto, isso vai ser descartado depois de implementar o alvy ray
			glm::mat4 rot(1.0f);
			rot = glm::rotate(rot, glm::radians(MainWindow->CameraRot.y), { 0.0f, 1.0f, 0.0f });
			rot = glm::rotate(rot, glm::radians(MainWindow->CameraRot.x), { 1.0f, 0.0f, 0.0f });
			rot = glm::inverse(rot);
			verticeTela = rot * glm::vec4({verticeTela, 0.0f});


			if (Renderer3D::UsePerspective) {
				glm::vec3 vertMundo = verticeTela;
				float zc = glm::max(vertMundo.z, 100.0f);

				verticeTela.x = ((vertMundo.x - MainWindow->Width * 0.5f) / zc) * 800.0f + (MainWindow->Width * 0.5f);
				verticeTela.y = ((vertMundo.y - MainWindow->Height * 0.5f) / zc) * 800.0f + (MainWindow->Height * 0.5f);
				verticeTela.z = -(1.0f / zc);
			}


			// aqui coloca a cor padrão do vértice ou o gouraudd
			if (LightAlgorithm == LightAlgorithms::Gouraud && meshTeste.GetAfectedByLights() && meshTeste.GetFill()) {

				glm::mat3 matrizTrans = meshTeste.GetTransformMatrix();
				matrizTrans = glm::inverseTranspose(matrizTrans);
				glm::vec3 vertexNormal = meshTeste.GetVertexNormals()[i];
				vertexNormal = matrizTrans * vertexNormal;
				vertexNormal = glm::normalize(vertexNormal);

				glm::vec3 resultColor = CalculateConstantLightning(vertexNormal, verticeTela, meshTeste.GetMaterial());
				currentVertices.push_back({ verticeTela, resultColor});

				interpolateVertices = true;
			}
			else if (LightAlgorithm == LightAlgorithms::Phong && meshTeste.GetAfectedByLights() && meshTeste.GetFill()) {
				glm::mat3 matrizTrans = meshTeste.GetTransformMatrix();
				matrizTrans = glm::inverseTranspose(matrizTrans);
				glm::vec3 vertexNormal = meshTeste.GetVertexNormals()[i];
				vertexNormal = matrizTrans * vertexNormal;
				vertexNormal = glm::normalize(vertexNormal);

				verticesNormals.push_back(vertexNormal);

				//glm::vec3 resultColor = CalculateConstantLightning(vertexNormal, allVertices[i], meshTeste.GetMaterial());
				currentVertices.push_back({ verticeTela, vertexNormal });

				interpolateVertices = true;
			}
			else {
				currentVertices.push_back({ verticeTela,{meshTeste.GetColor().x, meshTeste.GetColor().y, meshTeste.GetColor().z}});
			}

			verticesSum += verticeTela;

			if (verticeTela.y > minMaxFace.y) {
				minMaxFace.y = verticeTela.y;
			}
			if (verticeTela.y < minMaxFace.x) {
				minMaxFace.x = verticeTela.y;
			}
		}

		glm::vec3 centroide = verticesSum / static_cast<float>(currentVertices.size());

		if (centroide.x < 0 || centroide.y < 0 || centroide.x > MainWindow->Width || centroide.y > MainWindow->Height) {
			continue;
		}

		// Calcular iluminação constante
		if (meshTeste.GetAfectedByLights() && LightAlgorithm == LightAlgorithms::Constant && meshTeste.GetFill()) {
			glm::vec3 resultColor;
			resultColor = CalculateConstantLightning(meshNormal, centroide, meshTeste.GetMaterial());

			DrawColor = LeandroColor(resultColor.x, resultColor.y, resultColor.z, 1.0f);
		}else{
			DrawColor = LeandroColor(meshTeste.GetColor().x, meshTeste.GetColor().y, meshTeste.GetColor().z, 1.0f);
		}

		if (meshTeste.GetFill()) {
			FillPoly(currentVertices, minMaxFace, interpolateVertices, &meshTeste);
		}
		if (meshTeste.GetOutline()) {
			DrawColor = LeandroColor(meshTeste.GetOutlineColor().x, meshTeste.GetOutlineColor().y, meshTeste.GetOutlineColor().z, meshTeste.GetOutlineColor().w);
			Draw3DLinesWithVertices(currentVertices);
		}

	}

	// Aqui iria renderizar as meshes filhas
	// algo assim sla:
	//	for(Mesh* submesh : meshTeste->GetSubmeshes(){
	//		DrawMesh(submesh);
	//	}
}

glm::vec3 Renderer3D::CalculateConstantLightning(glm::vec3 normal, glm::vec3 centroide, MeshMaterial& material)
{
	MeshMaterial& currentMaterial = material;

	float ambientReflection = currentMaterial.ambientReflection;
	float diffuse = currentMaterial.diffuse;
	float specular = currentMaterial.specular;
	float specular_brightness = currentMaterial.specular_brightness;
	glm::vec3 baseColor = material.baseColor;

	// se o culling funcionar com >= 0 aqui deve ser z = -1, se não z = 1
	glm::vec3 observerVector = { 0.0f, 0.0f, -1.0f };

	glm::vec3 ambientResult = AmbientLight * ambientReflection * baseColor;
	glm::vec3 diffuseResult = { 0.0f, 0.0f, 0.0f };
	glm::vec3 specularResult = { 0.0f, 0.0f, 0.0f };

	for (MeshLight* light : *SceneLights) {
		float fatAtenuacao = 1.0f;
		float lightDistance = glm::distance(centroide, light->GetTransform().Position);

		if (light->GetLightType() != LightType::DirectionalLight) {
			//fatAtenuacao = glm::min(1.0f / (0.2f + (0.2f * lightDistance) + (0.2f * glm::pow(lightDistance, 2.0f))), 1.0f);
			fatAtenuacao = glm::pow(1.0f - glm::clamp(lightDistance / light->GetMaxDistance(), 0.0f, 1.0f), 2.0f);
			//if (lightDistance > light->GetMaxDistance()) { continue; }
		}
		glm::vec3 lightPos = light->GetTransform().Position;
		glm::vec3 lightIntensity = light->GetLightColor() * light->GetLightIntensity();

		glm::vec3 lightVector = lightPos - centroide;
		lightVector = glm::normalize(lightVector);

		float diffuseScalar = (glm::max(glm::dot(normal, lightVector), 0.0f));
		diffuseResult += diffuseScalar * lightIntensity * diffuse * baseColor * fatAtenuacao;

		if (diffuseScalar > 0.0f) {
			glm::vec3 specularR = glm::reflect(-lightVector, normal);
			specularResult += glm::pow(glm::max(glm::dot(specularR, observerVector), 0.0f), specular_brightness) * lightIntensity * specular;
		}
	}

	return glm::clamp(ambientResult + diffuseResult + specularResult, 0.0f, 1.0f);
}

void Renderer3D::FindIntersections(const std::vector<std::pair<glm::vec3, glm::vec3>>& vertices)
{
	for (int i = 0; i < vertices.size(); i++) {
		std::pair<glm::vec3, glm::vec3> sourceRaw = vertices[i];
		std::pair<glm::vec3, glm::vec3> targetRaw = vertices[(i + 1) % vertices.size()];

		// pega as cores
		glm::vec3 sourceColor = sourceRaw.second;
		glm::vec3 targetColor = targetRaw.second;

		// pega as posições
		glm::vec3 source = sourceRaw.first;
		glm::vec3 target = targetRaw.first;

		if (target.y == source.y) { continue; }

		// Aqui eu tenho os dois pontos
		if (target.y < source.y) { // point é mais alto que source; faz troca
			std::swap(source, target);
			std::swap(sourceColor, targetColor);
		}
		LE_CORE_ASSERT(target.y >= source.y, "Y FINAL da aresta maior que o INICIAL!");

		int ymin = static_cast<int>(std::ceil(source.y));
		int ymax = static_cast<int>(std::ceil(target.y));

		float dY = (target.y - source.y);

		float Tx = (target.x - source.x) / dY;
		float Tz = (target.z - source.z) / dY;

		float Tr = (targetColor.x - sourceColor.x) / dY;
		float Tg = (targetColor.y - sourceColor.y) / dY;
		float Tb = (targetColor.z - sourceColor.z) / dY;

		float new_x = source.x;
		float new_z = source.z;

		float new_r = sourceColor.x;
		float new_g = sourceColor.y;
		float new_b = sourceColor.z;

		float preStep = static_cast<float>(ymin) - source.y;
		new_x += preStep * Tx;
		new_z += preStep * Tz;

		new_r += preStep * Tr;
		new_g += preStep * Tg;
		new_b += preStep * Tb;

		// Pegar as interseções
		for (int y = ymin; y < ymax; y++) { // Scanlines de 0 à Ns-1
			//SDL_SetRenderDrawColorFloat(renderer, 1, 0, 0, SDL_ALPHA_OPAQUE_FLOAT);
			//SDL_RenderLine(renderer, new_x, ymin + i, new_x, ymin+i);
			//SDL_RenderPoint(renderer, new_x, ymin + i);
			if (y >= 0 && y < MainWindow->Height) {
				std::pair<glm::vec3, glm::vec3> intersectVertex;
				intersectVertex.first = {new_x, y, new_z};
				intersectVertex.second = { new_r, new_g, new_b };

				intersecoes[y].push_back(intersectVertex);

				//intersecoes[y].push_back({new_x, new_z});
				//LE_CORE_ASSERT(y > 0 && y < height, "Coordenada Y com valor INVALIDO!");
			}
			new_x += Tx;
			new_z += Tz;

			new_r += Tr;
			new_g += Tg;
			new_b += Tb;
		}
	}
}

void Renderer3D::FillPoly(const std::vector<std::pair<glm::vec3, glm::vec3>>& vertices, glm::vec2 minMaxY, bool interpolate, Mesh* mesh)
{
	int yMin = static_cast<int>(std::ceil(minMaxY.x));
	int yMax = static_cast<int>(std::ceil(minMaxY.y));

	if (vertices.size() < 3) { return; }

	//intersecoes.resize(yMax - yMin + 1); GARGALO FODIDO
	//LE_CORE_ASSERT(intersecoes.size() == height, "Intersecoes com tamanho incorreto alocado!");

	FindIntersections(vertices);

	/*Sub Polygons adicionados às interseções (talvez não seja a melhor forma de fazer, mas é uma forma)*/
	//for (BaseMesh* submesh : mesh->GetSubMeshes()) {
	//	std::vector<glm::vec3> sub_vertices = submesh->GetVertices();
	//	if (sub_vertices.size() < 3) { continue; }
	//	if (submesh->IsActive()) {
	//		ImVec4 old_color = mesh->GetColor();
	//		ImVec4 submesh_color = submesh->GetColor();
	//		SDL_SetRenderDrawColorFloat(renderer, submesh_color.x, submesh_color.y, submesh_color.z, SDL_ALPHA_OPAQUE_FLOAT);
	//		FillPoly(renderer, submesh, yMax - yMin + 1);
	//		SDL_SetRenderDrawColorFloat(renderer, old_color.x, old_color.y, old_color.z, SDL_ALPHA_OPAQUE_FLOAT);
	//	}
	//	FindIntersections(sub_vertices, intersecoes, yMax - yMin + 1, minMaxY);
	//}

	// Ordenar todas as interceções
	for (int i = yMin; i < yMax; i++) {
		if(i < 0 || i >= MainWindow->Height) { continue; }
		std::ranges::sort(intersecoes[i], 
		                  [](const std::pair<glm::vec3, glm::vec3>& a, const std::pair<glm::vec3, glm::vec3>& b) { return a.first.x < b.first.x; } // lambda pra comparar os dois
		);
	}

	// Fazer o fill no poly
	for (int y = yMin; y < yMax; y++) {
		if (y < 0 || y >= MainWindow->Height) { continue; }
		std::vector<std::pair<glm::vec3, glm::vec3>>& lista = intersecoes[y];

		for (int i = 0; i + 1 < lista.size(); i += 2) {
			glm::vec3 source = lista[i].first;
			glm::vec3 target = lista[i + 1].first;
			
			glm::vec3 sourceColor = lista[i].second;
			glm::vec3 targetColor = lista[i + 1].second;

			float dx = target.x - source.x;
			float Tz = (target.z - source.z) / dx;
			
			float Tr = (targetColor.x - sourceColor.x) / dx;
			float Tg = (targetColor.y - sourceColor.y) / dx;
			float Tb = (targetColor.z - sourceColor.z) / dx;

			float currentZ = source.z;
			float currentR = sourceColor.x;
			float currentG = sourceColor.y;
			float currentB = sourceColor.z;

			int xStart = static_cast<int>(std::ceil(source.x));
			int xEnd = static_cast<int>(std::ceil(target.x));

			currentZ += (static_cast<float>(xStart) - source.x) * (dx > 0.0001f ? Tz : 1.0f);
			currentR += (static_cast<float>(xStart) - source.x) * (dx > 0.0001f ? Tr : 1.0f);
			currentG += (static_cast<float>(xStart) - source.x) * (dx > 0.0001f ? Tg : 1.0f);
			currentB += (static_cast<float>(xStart) - source.x) * (dx > 0.0001f ? Tb : 1.0f);
			// utilizando o buffer
			for (int x = xStart; x < xEnd; x++) {
				if (x < 0 || x >= MainWindow->Width) { 
					currentZ += (dx > 0.0001f ? Tz : 0.0f);

					currentR += (dx > 0.0001f ? Tr : 0.0f);
					currentG += (dx > 0.0001f ? Tg : 0.0f);
					currentB += (dx > 0.0001f ? Tb : 0.0f);
					continue; }
				if (y < 0 || y >= MainWindow->Height) { 
					currentZ += (dx > 0.0001f ? Tz : 0.0f);

					currentR += (dx > 0.0001f ? Tr : 0.0f);
					currentG += (dx > 0.0001f ? Tg : 0.0f);
					currentB += (dx > 0.0001f ? Tb : 0.0f);
					continue; }

				if (currentZ <= ZBuffer->at(y * MainWindow->Width + x)) {
					// Phong a cada pixel né?
					//glm::vec3 currentColor = CalculateConstantLightning();
					if (Renderer3D::LightAlgorithm == LightAlgorithms::Phong && mesh->GetAfectedByLights()) {
						glm::vec3 N = { currentR, currentG, currentB };
						N = glm::normalize(N);
						glm::vec3 resultColor = CalculateConstantLightning(N, { x, y, currentZ }, mesh->GetMaterial());

						Buffer->at(y * MainWindow->Width + x) = LeandroColor(resultColor.x, resultColor.y, resultColor.z, 1.0f);
					}
					else {
						Buffer->at(y * MainWindow->Width + x) = interpolate ? LeandroColor(glm::clamp(currentR, 0.0f, 1.0f), glm::clamp(currentG, 0.0f, 1.0f), glm::clamp(currentB, 0.0f, 1.0f), 1.0f) : DrawColor;
					}
					ZBuffer->at(y * MainWindow->Width + x) = currentZ;
				}
				currentZ += (dx > 0.0001f ? Tz : 0.0f);

				currentR += (dx > 0.0001f ? Tr : 0.0f);
				currentG += (dx > 0.0001f ? Tg : 0.0f);
				currentB += (dx > 0.0001f ? Tb : 0.0f);
			}
		}
		intersecoes[y].clear();
	}
}