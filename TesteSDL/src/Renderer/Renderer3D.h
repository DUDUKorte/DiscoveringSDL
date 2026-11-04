#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include "EditorCore/Mesh.h"
#include "EditorApp.h"
#include "Utils/Log.h"

// descomente essa linha apenas se for usar o novo renderer por hardware
// (precisa ter o SDK do vulkan devidamente instalado)
#define USE_HARDWARE_RENDERER

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


#ifdef USE_HARDWARE_RENDERER
	// função pra compilar shader de glsl pra spir-v
	static SDL_GPUShader* CompileGLSLToSDLShader(
		SDL_GPUDevice* device,
		const char* source,
		shaderc_shader_kind kind,
		SDL_GPUShaderStage stage,
		uint32_t numUniformBuffers)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetOptimizationLevel(shaderc_optimization_level_performance);

		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, strlen(source), kind, "shader", options);
		if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
			SDL_Log("Erro compilando Shader: %s", module.GetErrorMessage().c_str());
			return nullptr;
		}

		std::vector<uint32_t> spirv(module.cbegin(), module.cend());

		SDL_GPUShaderCreateInfo shaderInfo = {};
		shaderInfo.code = reinterpret_cast<const uint8_t*>(spirv.data());
		shaderInfo.code_size = spirv.size() * sizeof(uint32_t);
		shaderInfo.entrypoint = "main";
		shaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
		shaderInfo.stage = stage;
		shaderInfo.num_uniform_buffers = numUniformBuffers;

		return SDL_CreateGPUShader(device, &shaderInfo);
	}
#else // isso é só pra evitar erro caso a pessoa n tenha o sdk do vulkan e n queira instalar (aka vai usar/codar só no sftware renderer)
	typedef enum {shaderc_vertex_shader,shaderc_fragment_shader,shaderc_compute_shader,shaderc_geometry_shader,shaderc_tess_control_shader,shaderc_tess_evaluation_shader,shaderc_glsl_vertex_shader = shaderc_vertex_shader,shaderc_glsl_fragment_shader = shaderc_fragment_shader,shaderc_glsl_compute_shader = shaderc_compute_shader,shaderc_glsl_geometry_shader = shaderc_geometry_shader,shaderc_glsl_tess_control_shader = shaderc_tess_control_shader,shaderc_glsl_tess_evaluation_shader = shaderc_tess_evaluation_shader,shaderc_glsl_infer_from_source,shaderc_glsl_default_vertex_shader,shaderc_glsl_default_fragment_shader,shaderc_glsl_default_compute_shader,shaderc_glsl_default_geometry_shader,shaderc_glsl_default_tess_control_shader,shaderc_glsl_default_tess_evaluation_shader,shaderc_spirv_assembly,shaderc_raygen_shader,shaderc_anyhit_shader,shaderc_closesthit_shader,shaderc_miss_shader,shaderc_intersection_shader,shaderc_callable_shader,shaderc_glsl_raygen_shader = shaderc_raygen_shader,shaderc_glsl_anyhit_shader = shaderc_anyhit_shader,shaderc_glsl_closesthit_shader = shaderc_closesthit_shader,shaderc_glsl_miss_shader = shaderc_miss_shader,shaderc_glsl_intersection_shader = shaderc_intersection_shader,shaderc_glsl_callable_shader = shaderc_callable_shader,shaderc_glsl_default_raygen_shader,shaderc_glsl_default_anyhit_shader,shaderc_glsl_default_closesthit_shader,shaderc_glsl_default_miss_shader,shaderc_glsl_default_intersection_shader,shaderc_glsl_default_callable_shader,shaderc_task_shader,shaderc_mesh_shader,shaderc_glsl_task_shader = shaderc_task_shader,shaderc_glsl_mesh_shader = shaderc_mesh_shader,shaderc_glsl_default_task_shader,shaderc_glsl_default_mesh_shader,} shaderc_shader_kind;
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
