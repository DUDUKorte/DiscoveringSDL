#include "Renderer3D.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include "Utils/LeandroMath.h"
#include <glm/gtc/random.hpp>

std::vector<uint32_t>* Renderer3D::Buffer = nullptr;
std::vector<float>* Renderer3D::ZBuffer = nullptr;
std::vector<MeshLight*>* Renderer3D::SceneLights = nullptr;
WindowProperties* Renderer3D::MainWindow = nullptr;
uint32_t Renderer3D::DrawColor = LeandroColor(0.8f, 0.8f, 0.2f, 1.0f);
std::vector<std::vector<std::pair<glm::vec3, glm::vec3>>> Renderer3D::intersecoes(0);
glm::vec3 Renderer3D::AmbientLight = { 0.2f, 0.2f, 0.2f };
LightAlgorithms Renderer3D::LightAlgorithm = Constant;
SDL_GPUTexture* Renderer3D::DepthTexture = nullptr;

#ifndef USE_HARDWARE_RENDERER
RendererModes Renderer3D::RendererMode = RendererModes::Software;
#else
RendererModes Renderer3D::RendererMode = RendererModes::Hardware;
#endif

const char* vertexShaderSource = R"(
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 cameraPos;

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

void Renderer3D::InitGPU(WindowProperties* window)
{
	Renderer3D::MainWindow = window;

	SDL_GPUTextureCreateInfo depthInfo = {};
	depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
	depthInfo.width = MainWindow->Width;
	depthInfo.height = MainWindow->Height;
	depthInfo.layer_count_or_depth = 1;
	depthInfo.num_levels = 1;
	depthInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
	depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
	DepthTexture = SDL_CreateGPUTexture(MainWindow->GPUDevice, &depthInfo);

	// SHader compilation
	SDL_GPUShader* vertShader = CompileGLSLToSDLShader(MainWindow->GPUDevice, vertexShaderSource, shaderc_glsl_vertex_shader, SDL_GPU_SHADERSTAGE_VERTEX, 2);
	SDL_GPUShader* fragShader = CompileGLSLToSDLShader(MainWindow->GPUDevice, fragmentShaderSource, shaderc_glsl_fragment_shader, SDL_GPU_SHADERSTAGE_FRAGMENT, 1);

	// Layout dos Vértices (GPUVertex: vec3 pos + vec3 normal)
	SDL_GPUVertexBufferDescription vertexBufferDesc = {};
	vertexBufferDesc.slot = 0;
	vertexBufferDesc.pitch = sizeof(GPUVertex);
	vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

	SDL_GPUVertexAttribute vertexAttributes[2] = {};
	// position (offset 0)
	vertexAttributes[0].location = 0;
	vertexAttributes[0].buffer_slot = 0;
	vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	vertexAttributes[0].offset = offsetof(GPUVertex, position);

	// normal (offset 12)
	vertexAttributes[1].location = 1;
	vertexAttributes[1].buffer_slot = 0;
	vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	vertexAttributes[1].offset = offsetof(GPUVertex, normal);

	// Target de Cor e Depth Stencil
	SDL_GPUColorTargetDescription colorTargetDesc = {};
	colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(MainWindow->GPUDevice, MainWindow->Window);

	SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.vertex_shader = vertShader;
	pipelineInfo.fragment_shader = fragShader;
	pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

	pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &vertexBufferDesc;
	pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
	pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;
	pipelineInfo.vertex_input_state.num_vertex_attributes = 2;

	// Backface Culling
	pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE; // Podemos ligar BACK/FRONT depois de testar a mão da matriz

	// Z-Buffer
	pipelineInfo.depth_stencil_state.enable_depth_test = true;
	pipelineInfo.depth_stencil_state.enable_depth_write = true;
	pipelineInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

	pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
	pipelineInfo.target_info.num_color_targets = 1;
	pipelineInfo.target_info.has_depth_stencil_target = true;
	pipelineInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

	MainWindow->GPUPipeline = SDL_CreateGPUGraphicsPipeline(MainWindow->GPUDevice, &pipelineInfo);

	// Depois de criar o pipeline, o SDL_GPU permite liberar os objetos de shader
	SDL_ReleaseGPUShader(MainWindow->GPUDevice, vertShader);
	SDL_ReleaseGPUShader(MainWindow->GPUDevice, fragShader);
}

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

	// criar vertex buffer
	SDL_GPUBufferCreateInfo bufInfo = {};
	bufInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	bufInfo.size = byteSize;
	mesh.SetGPUBuffer(SDL_CreateGPUBuffer(MainWindow->GPUDevice, &bufInfo));

	// cria transfer buffer
	SDL_GPUTransferBufferCreateInfo transInfo = {};
	transInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	transInfo.size = byteSize;
	SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(MainWindow->GPUDevice, &transInfo);

	void* mapPtr = SDL_MapGPUTransferBuffer(MainWindow->GPUDevice, transferBuffer, false);
	memcpy(mapPtr, gpuVertices.data(), byteSize);
	SDL_UnmapGPUTransferBuffer(MainWindow->GPUDevice, transferBuffer);

	// copy pass pra gpu
	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(MainWindow->GPUDevice);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

	SDL_GPUTransferBufferLocation srcLoc = {};
	srcLoc.transfer_buffer = transferBuffer;
	srcLoc.offset = 0;

	SDL_GPUBufferRegion dstReg = {};
	dstReg.buffer = mesh.GetGPUBuffer();
	dstReg.offset = 0;
	dstReg.size = byteSize;

	SDL_UploadToGPUBuffer(copyPass, &srcLoc, &dstReg, false);
	SDL_EndGPUCopyPass(copyPass);
	SDL_SubmitGPUCommandBuffer(commandBuffer);

	SDL_ReleaseGPUTransferBuffer(MainWindow->GPUDevice, transferBuffer);
}

void Renderer3D::DrawMeshGPU(Mesh& mesh, SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass)
{
	// só em via das dúvidas
	if (!mesh.GetGPUBuffer())
	{
		UploadMeshToGPU(mesh);
		if (!mesh.GetGPUBuffer())
		{
			LE_CORE_CRITICAL("ERRO: Mesh incompatível com Hardware Renderer!!! Verifique se seu modelo 3D está triangularizado!!!!");
			return;
		}
	}

	// apeando coordenadas de tela
	VertexUniformData vertUbo = {};
	vertUbo.modelMatrix = mesh.GetTransformMatrix();
	vertUbo.normalMatrix = glm::transpose(glm::inverse(vertUbo.modelMatrix));

	// projeção ortográfica
	glm::mat4 projMatrix;
	glm::mat4 viewMatrix(1.0f);
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), MainWindow->CameraPos);
	transform = glm::rotate(transform, glm::radians(MainWindow->CameraRot.y), { 0.0f, 1.0f, 0.0f });
	transform = glm::rotate(transform, glm::radians(MainWindow->CameraRot.x), { 1.0f, 0.0f, 0.0f });

	if (mesh.GetUsePerspective()) {
		projMatrix = glm::perspectiveLH_ZO(glm::radians(60.0f), (float)MainWindow->Width/MainWindow->Height, 0.1f, 100000.0f);
		projMatrix[1][1] *= -1.0f; // arruma o y
	}
	else {
		projMatrix = glm::ortho(
			0.0f, (float)MainWindow->Width,
			(float)MainWindow->Height, 0.0f,
			10000.0f, -10000.0f
		);
	}

	viewMatrix = glm::inverse(transform);
	
	//viewProj = glm::translate(viewProj, MainWindow->CameraPos); // só funciona pra câmera 2D
	vertUbo.cameraMatrix = glm::mat4(1.0f);
	//vertUbo.cameraMatrix = glm::translate(vertUbo.cameraMatrix, MainWindow->CameraPos);

	vertUbo.viewProjMatrix = projMatrix * viewMatrix;

	// dados de material
	MeshMaterial& mat = mesh.GetMaterial();
	FragmentUniformData fragUbo = {};
	fragUbo.baseColor = glm::vec4(mat.baseColor, 1.0f);
	fragUbo.materialProps = glm::vec4(mat.ambientReflection, mat.diffuse, mat.specular, mat.specular_brightness);
	fragUbo.ambientLight = glm::vec4(AmbientLight, 1.0f);
	fragUbo.lightAlgorithm = static_cast<int>(Renderer3D::LightAlgorithm);

	if (SceneLights && mesh.GetAfectedByLights()) {
		int count = std::min((int)SceneLights->size(), MAX_LIGHTS);
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

	if (mesh.GetUsePerspective()) {
		fragUbo.perspective = 1;
	}
	else {
		fragUbo.perspective = 0;
	}

	SDL_PushGPUVertexUniformData(commandBuffer, 0, &vertUbo, sizeof(VertexUniformData));
	SDL_PushGPUVertexUniformData(commandBuffer, 1, &fragUbo, sizeof(FragmentUniformData));
	SDL_PushGPUFragmentUniformData(commandBuffer, 0, &fragUbo, sizeof(FragmentUniformData));

	// faz o Bind do vertex buffer da mesh e desenha
	SDL_GPUBufferBinding binding = {};
	binding.buffer = mesh.GetGPUBuffer();
	binding.offset = 0;
	SDL_BindGPUVertexBuffers(renderPass, 0, &binding, 1);

	SDL_DrawGPUPrimitives(renderPass, mesh.GetVertexCount(), 1, 0, 0);

}

void Renderer3D::Init(std::vector<uint32_t>* buffer, std::vector<float>* zbuffer, WindowProperties* window, std::vector<MeshLight*>* lights)
{
	Buffer = buffer;
	ZBuffer = zbuffer;
	SceneLights = lights;
	MainWindow = window;
	Renderer3D::intersecoes = std::vector<std::vector<std::pair<glm::vec3, glm::vec3>>>(MainWindow->Height);
}

void Renderer3D::UpdateResize() {
	Renderer3D::intersecoes = std::vector<std::vector<std::pair<glm::vec3, glm::vec3>>>(MainWindow->Height);
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

	float steps = (float)((int)std::ceil(std::max(std::abs(dX), std::abs(dY))));

	if (steps == 0) {
		int x = (int)std::lround(source.x);
		int y = (int)std::lround(source.y);
		int z = (int)std::lround(source.z);
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
	int xStart = (int)std::ceil(source.x);
	currentZ += ((float)xStart - source.x) * (dX > 0.0001f ? zInc : 1.0f);
	
	for (int i = 0; i <= steps; i++) {   // <= inclui o ponto final
		int px = (int)std::lround(new_x);
		int py = (int)std::lround(new_y);

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

			if (meshTeste.GetUsePerspective()) {
				glm::vec3 vertMundo = allVertices[i];
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

		glm::vec3 centroide = verticesSum / (float)currentVertices.size();

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

void Renderer3D::FindIntersections(const std::vector<std::pair<glm::vec3, glm::vec3>>& vertices, glm::vec2 yMinMax)
{
	int meshYMin = int(yMinMax.x);
	int meshYMax = int(yMinMax.y);

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

		int ymin = (int)std::ceil(source.y);
		int ymax = (int)std::ceil(target.y);

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

		float preStep = (float)ymin - source.y;
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
	int yMin = (int)std::ceil(minMaxY.x);
	int yMax = (int)std::ceil(minMaxY.y);

	if (vertices.size() < 3) { return; }

	//intersecoes.resize(yMax - yMin + 1); GARGALO FODIDO
	//LE_CORE_ASSERT(intersecoes.size() == height, "Intersecoes com tamanho incorreto alocado!");

	FindIntersections(vertices, minMaxY);

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
		std::sort(intersecoes[i].begin(), intersecoes[i].end(), 
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
			float dy = target.y - source.y;
			float Tz = (target.z - source.z) / dx;
			
			float Tr = (targetColor.x - sourceColor.x) / dx;
			float Tg = (targetColor.y - sourceColor.y) / dx;
			float Tb = (targetColor.z - sourceColor.z) / dx;

			float currentZ = source.z;
			float currentR = sourceColor.x;
			float currentG = sourceColor.y;
			float currentB = sourceColor.z;

			int xStart = (int)std::ceil(source.x);
			int xEnd = (int)std::ceil(target.x);

			currentZ += ((float)xStart - source.x) * (dx > 0.0001f ? Tz : 1.0f);
			currentR += ((float)xStart - source.x) * (dx > 0.0001f ? Tr : 1.0f);
			currentG += ((float)xStart - source.x) * (dx > 0.0001f ? Tg : 1.0f);
			currentB += ((float)xStart - source.x) * (dx > 0.0001f ? Tb : 1.0f);
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