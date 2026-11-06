#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include "EditorCore/Mesh.h"
#include "EditorApp.h"
#include "Utils/Log.h"

// descomente essa linha apenas se for usar o novo renderer por hardware
// (precisa ter o SDK do vulkan devidamente instalado)
// vale comentar que precisa incluir/remover shaderc_shared.lib nas inclusões do projeto (tanto na engine quanto na application)
// se for buildar release com suporte a vulkan precisa (logicamente) buildar num pc com o vulkan SDK instalado e USE_HARDWARE_RENDERER definido
// mas também precisa colocar o shaderc_shared.lib ou shaderc_shared.dll (só testei com o .dll) na pasta da build
// (ou pedir pro usuário instalar o vulkanSDK skdfjdklsfjldks)
#define USE_HARDWARE_RENDERER

// Lembra de trocar esse valor no **fragment** >>>E<<< **vertex** shader tbm se n vai crashar
#define MAX_LIGHTS 16

#ifdef USE_HARDWARE_RENDERER
#include <shaderc/shaderc.hpp>
#endif

#ifndef LeandroColor
#define LeandroColor(r, g, b, a) \
	( ((uint32_t) ((a) * 255.0f + 0.5f) << 24) | \
	((uint32_t) ((b) * 255.0f + 0.5f) << 16) | \
	((uint32_t) ((g) * 255.0f + 0.5f) << 8) | \
	((uint32_t) ((r) * 255.0f + 0.5f)))
#endif

struct GPUVertex {
	glm::vec3 position;
	glm::vec3 normal;
};

struct GPULight {
	glm::vec4 posAndType;
	glm::vec4 colorAndMaxDist;
};

struct VertexUniformData {
	glm::mat4 modelMatrix;
	glm::mat4 viewProjMatrix;
	glm::mat4 normalMatrix;
	glm::mat4 cameraMatrix;
};

// Uniform do Fragment Shader (Material + Luzes)
struct FragmentUniformData {
	glm::vec4 baseColor;
	glm::vec4 materialProps; // x = ambient, y = diffuse, z = specular, w = n
	glm::vec4 ambientLight;
	int lightAlgorithm;
	int numLights;
	int perspective, pad1;
	GPULight lights[MAX_LIGHTS];
};

// @Mattsu TODO: Depois de fazer a triangulação das meshes tenta implementar um FastPhong (precisa ser com mesh triangulada)
// Talvez seja melhor tentar fazer o fastphong depois de tu refatorar/reescrever o fillpoly, já q ele vai mudar p krl...
enum LightAlgorithms { Constant = 0, Gouraud = 1, Phong };
enum RendererModes { Software = 0, Hardware = 1 };

class Renderer3D {

public:
	static void InitGPU(WindowProperties* window);

	static void CreateGPUPipeline();

	static void UploadMeshToGPU(Mesh& mesh);

	static void DrawMeshGPU(Mesh& mesh, SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass);
	
	static void Init(std::vector<uint32_t>* buffer, std::vector<float>* zbuffer, WindowProperties* window, std::vector<MeshLight*>* lights);

	static void UpdateResize();

	static void DrawLine(glm::vec3 source, glm::vec3 target);

	static void Draw3DLinesWithVertices(const std::vector<std::pair<glm::vec3, glm::vec3>>& vertices);

	static void DrawMesh(Mesh& meshTeste);

	static glm::vec3 CalculateConstantLightning(glm::vec3 normal, glm::vec3 centroide, MeshMaterial& material);

	static void FindIntersections(const std::vector<std::pair<glm::vec3, glm::vec3>>& vertices, glm::vec2 yMinMax);	

	// @Mattsu TODO: Precisamos de um FillPoly pica, novo, primeiro tem que fazer a triangularização das meshes importadas
	// aí da pra fazer o fill poly determinístico, basicamente não precisa ordenar lista, nem rodar um y que vá pra y+1
	// resumindo: sendo tudo triangulo da pra pegar qualquer ponto e descobrir a interpolação, o x1 e x2, etc...
	// O lance é que não vamos usar a CPU, até poderia (talvez usar o OpenMP pra multithread)
	// Mas a ideia é usar compute shading, cada núcleo da GPU cuida de 1 linha do fillpoly em simultâneo
	// então vai ficar rápido p krll, como toda GPU, até as mais básicas tem mais de 720 núcleos pra rodar isso
	// vai ser bem rápido, o tempo de 1 calcular termina o fill inteiro dql poly
	static void FillPoly(const std::vector<std::pair<glm::vec3, glm::vec3>>& vertices, glm::vec2 minMaxY, bool interpolate = false, Mesh* mesh = nullptr);

	static std::vector<uint32_t>* Buffer;
	static std::vector<float>* ZBuffer;
	static std::vector<MeshLight*>* SceneLights;
	static WindowProperties* MainWindow;
	static uint32_t DrawColor;
	static std::vector<std::vector<std::pair<glm::vec3, glm::vec3>>> intersecoes;
	static glm::vec3 AmbientLight;
	static LightAlgorithms LightAlgorithm;

	static RendererModes RendererMode;
	static SDL_GPUTexture* DepthTexture;
	static bool UsePerspective;
	static bool WireframeMode;

	static SDL_GPUShader* m_VertexShader;
	static SDL_GPUShader* m_FragmentShaderFill;
	static SDL_GPUShader* m_FragmentShaderWireframe;

	static SDL_GPUGraphicsPipeline* DefaultPipeline;
	static SDL_GPUGraphicsPipeline* WireframePipeline;

#ifdef USE_HARDWARE_RENDERER
	// função pra compilar shader de glsl pra spir-v (binário do vulkan)
	// eu mantive essa função no .h mesmo pq o .cpp tava muito poluído
	// e já aproveita aqui pra fazer o ifdef e tal, pq aí essa função nem chega a existir
	// ent aí permite que que pessoas sem o VulkanSDK instalado possam buildar usando só o software renderer
	// td comentado aqui já
	static SDL_GPUShader* CompileGLSLToSDLShader(
		SDL_GPUDevice* device,
		const char* source,	// o shader glsl em si
		shaderc_shader_kind kind, // tipo do shader == vertex / fragment. tem MUITOS outros (inclusive isso aqui é do SDK do vulkan), mas o principal é esses dois. Provavelmente deve ser mudando isso pra fazer um shader de ray tracing ou coisa do tipo
		SDL_GPUShaderStage stage, // mesma coisa do tipo, mas pro SDL (esse sim só tem vertex e fragment)
		uint32_t numUniformBuffers) // Quantidade de uniform buffers vamos 'upar' no shader (VertexUniformData/FragmentUniformData)
									// como o bglh roda na gpu n da pra simplesmente fazer 'Shdaer(data1, data2)' tem q mandar em bytes mesmo e precisa ALINHAR AINDA, chato, mas a vida é assim
	{
		shaderc::Compiler compiler; // compiler nativo do próprio vulkan SDK (MUITO OBRIGADO VULKAN MDS DO CÉU)
		shaderc::CompileOptions options; //dã
		options.SetOptimizationLevel(shaderc_optimization_level_performance); // FPS ALTO KJASHDJKHAGHFRHJKLAWHGELRJKAW
																			  // Aqui spa dava pra fazer algo tipo #ifdef OPTIMIZATIONS e aí n usa caso esteja rodando em debug
																			  // mas sinceramente n vejo necessidade em não ligar a performance na compilação de shader
																			  // ainda mais pq nossos shader são MUITO simples (me aguerde RAY TRACING)

		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, strlen(source), kind, "shader", options); // essa linha literalmente compila o shader todo, olha q delícia
		if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
			SDL_Log("Erro compilando Shader: %s", module.GetErrorMessage().c_str());
			return nullptr;
		}
		// daqui pra frente é configurando o shader pro sdl conseguir usar e upar na gpu, o shader já tá compilado
		std::vector<uint32_t> spirv(module.cbegin(), module.cend());

		SDL_GPUShaderCreateInfo shaderInfo = {};
		shaderInfo.code = reinterpret_cast<const uint8_t*>(spirv.data()); // o literal código do shader compilado
		shaderInfo.code_size = spirv.size() * sizeof(uint32_t);
		shaderInfo.entrypoint = "main"; // função de entrada (ideia: colocar LeandroInit() ou LEMain() ??)
		shaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV; // avisar q é SPIRV né, se n vai crashar
		shaderInfo.stage = stage; // vertex/fragment
		shaderInfo.num_uniform_buffers = numUniformBuffers;

		return SDL_CreateGPUShader(device, &shaderInfo); //shader SDL criado com sucesso óia só q coisa legal, fizemos um compilador de shader vulkan :D
	}
#else // isso é só pra evitar erro caso a pessoa n tenha o sdk do vulkan e n queira instalar (aka vai usar/codar só no sftware renderer)
	typedef enum {shaderc_vertex_shader,shaderc_fragment_shader,shaderc_compute_shader,shaderc_geometry_shader,shaderc_tess_control_shader,shaderc_tess_evaluation_shader,shaderc_glsl_vertex_shader = shaderc_vertex_shader,shaderc_glsl_fragment_shader = shaderc_fragment_shader,shaderc_glsl_compute_shader = shaderc_compute_shader,shaderc_glsl_geometry_shader = shaderc_geometry_shader,shaderc_glsl_tess_control_shader = shaderc_tess_control_shader,shaderc_glsl_tess_evaluation_shader = shaderc_tess_evaluation_shader,shaderc_glsl_infer_from_source,shaderc_glsl_default_vertex_shader,shaderc_glsl_default_fragment_shader,shaderc_glsl_default_compute_shader,shaderc_glsl_default_geometry_shader,shaderc_glsl_default_tess_control_shader,shaderc_glsl_default_tess_evaluation_shader,shaderc_spirv_assembly,shaderc_raygen_shader,shaderc_anyhit_shader,shaderc_closesthit_shader,shaderc_miss_shader,shaderc_intersection_shader,shaderc_callable_shader,shaderc_glsl_raygen_shader = shaderc_raygen_shader,shaderc_glsl_anyhit_shader = shaderc_anyhit_shader,shaderc_glsl_closesthit_shader = shaderc_closesthit_shader,shaderc_glsl_miss_shader = shaderc_miss_shader,shaderc_glsl_intersection_shader = shaderc_intersection_shader,shaderc_glsl_callable_shader = shaderc_callable_shader,shaderc_glsl_default_raygen_shader,shaderc_glsl_default_anyhit_shader,shaderc_glsl_default_closesthit_shader,shaderc_glsl_default_miss_shader,shaderc_glsl_default_intersection_shader,shaderc_glsl_default_callable_shader,shaderc_task_shader,shaderc_mesh_shader,shaderc_glsl_task_shader = shaderc_task_shader,shaderc_glsl_mesh_shader = shaderc_mesh_shader,shaderc_glsl_default_task_shader,shaderc_glsl_default_mesh_shader,} shaderc_shader_kind;
	// Função fantasma quando USE_HARDWARE_RENDERER não foi definido
	static SDL_GPUShader* CompileGLSLToSDLShader(
		SDL_GPUDevice* device,
		const char* source,
		shaderc_shader_kind kind,
		SDL_GPUShaderStage stage,
		uint32_t numUniformBuffers) {
		return nullptr;
	}
#endif
};
