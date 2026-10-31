#include "Renderer3D.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <glm/gtc/random.hpp>

std::vector<uint32_t>* Renderer3D::Buffer = nullptr;
std::vector<float>* Renderer3D::ZBuffer = nullptr;
std::vector<MeshLight*>* Renderer3D::SceneLights = nullptr;
WindowProperties* Renderer3D::MainWindow = nullptr;
uint32_t Renderer3D::DrawColor = LeandroColor(0.8f, 0.8f, 0.2f, 1.0f);
std::vector<std::vector<std::pair<float, float>>> Renderer3D::intersecoes(0);
glm::vec3 Renderer3D::AmbientLight = { 0.2f, 0.2f, 0.2f };

void Renderer3D::Init(std::vector<uint32_t>* buffer, std::vector<float>* zbuffer, WindowProperties* window, std::vector<MeshLight*>* lights)
{
	Buffer = buffer;
	ZBuffer = zbuffer;
	SceneLights = lights;
	MainWindow = window;
	Renderer3D::intersecoes = std::vector<std::vector<std::pair<float, float>>>(MainWindow->Height);
}

void Renderer3D::UpdateResize() {
	Renderer3D::intersecoes = std::vector<std::vector<std::pair<float, float>>>(MainWindow->Height);
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

	float steps = (int)std::ceil(std::max(std::abs(dX), std::abs(dY)));

	if (steps == 0) {
		int x = (int)std::lround(source.x);
		int y = (int)std::lround(source.y);
		if (x >= 0 && x < width && y >= 0 && y < height) {
			Buffer->at(y * MainWindow->Width + x) = DrawColor;
		}
		return;
	}

	float xInc = dX / steps;
	float yInc = dY / steps;

	float new_x = source.x;
	float new_y = source.y;

	for (int i = 0; i <= steps; i++) {   // <= inclui o ponto final
		int px = (int)std::lround(new_x);
		int py = (int)std::lround(new_y);

		if (px >= 0 && px < width && py >= 0 && py < height) {
			Buffer->at(py * MainWindow->Width + px) = DrawColor;
		}

		new_x += xInc;
		new_y += yInc;
	}

	//int ymin = (int)source.y;
	//int ymax = (int)target.y;
	//for (int y = ymin; y < ymax; y++) {
	//	vertexBuffer->at(y * 1280 + int(new_x)) = LeandroColor(0.2f, 0.2f, 0.8, 1.0);
	//	new_x += Tx;
	//}
}

void Renderer3D::Draw3DLinesWithVertices(const std::vector<glm::vec3>& vertices)
{
	if (vertices.size() < 2) { // só desenha se tiver 2 ou mais vertices
		Buffer->at(vertices[0].y * MainWindow->Width + vertices[0].x) = DrawColor;
		//SDL_RenderPoint(renderer, vertices[0].x, vertices[0].y);
		return;
	}

	for (int i = 0; i < vertices.size() - 1; i++) {

		DrawLine(vertices[i], vertices[i + 1]);
		//SDL_RenderLine(renderer, vertices[i].x, vertices[i].y, vertices[i + 1].x, vertices[i + 1].y);

		if (vertices.size() - 2 == i) {
			//SDL_RenderLine(renderer, vertices[i + 1].x, vertices[i + 1].y, vertices[0].x, vertices[0].y);
			DrawLine(vertices[i + 1], vertices[0]);
		}
	}
}

void Renderer3D::DrawMesh(Mesh& meshTeste)
{
	std::vector<glm::vec3>& allVertices = meshTeste.GetVertices();
	std::vector<glm::vec3>& meshNormals = meshTeste.GetNormals();
	std::vector<glm::vec3> currentVertices = {};
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


		minMaxFace = { 10000.0f, -1.0f };
		currentVertices.clear();
		glm::vec3 verticesSum = { 0.0f, 0.0f, 0.0f };

		for (int i : currentFace.vertices) {
			glm::vec3 verticeTela = allVertices[i];

			if (meshTeste.GetUsePerspective()) {
				glm::vec3 vertMundo = allVertices[i];
				float zc = glm::max(vertMundo.z, 100.0f);

				verticeTela.x = ((vertMundo.x - MainWindow->Width * 0.5f) / zc) * 800.0f + (MainWindow->Width * 0.5f);
				verticeTela.y = ((vertMundo.y - MainWindow->Width * 0.5f) / zc) * 800.0f + (MainWindow->Height* 0.5f);
				verticeTela.z = -(1.0f / zc);
			}

			currentVertices.push_back(verticeTela);
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
		if (meshTeste.GetAfectedByLights()) {
			

			MeshMaterial& currentMaterial = meshTeste.GetMaterial();

			float ambientReflection = currentMaterial.ambientReflection;
			float diffuse = currentMaterial.diffuse;
			float specular = currentMaterial.specular;
			float specular_brightness = currentMaterial.specular_brightness;
			glm::vec3 baseColor = { meshTeste.GetColor().x, meshTeste.GetColor().y, meshTeste.GetColor().z };

			// se o culling funcionar com >= 0 aqui deve ser z = -1, se não z = 1
			glm::vec3 observerVector = { 0.0f, 0.0f, -1.0f };

			glm::vec3 ambientResult = AmbientLight * ambientReflection * baseColor;
			glm::vec3 diffuseResult = {0.0f, 0.0f, 0.0f};
			glm::vec3 specularResult = { 0.0f, 0.0f, 0.0f };

			for (MeshLight* light : *SceneLights) {
				if (glm::distance(centroide, light->GetTransform().Position) > light->GetMaxDistance()) {
					continue;
				}
				glm::vec3 lightPos = light->GetTransform().Position;
				glm::vec3 lightIntensity = light->GetLightColor() * light->GetLightIntensity();
				
				glm::vec3 lightVector = lightPos - centroide;
				lightVector = glm::normalize(lightVector);
				
				float diffuseScalar = (glm::max(glm::dot(meshNormal, lightVector), 0.0f));
				diffuseResult += diffuseScalar * lightIntensity * diffuse * baseColor;
				
				if (diffuseScalar > 0.0f) {
					glm::vec3 specularR = glm::reflect(-lightVector, meshNormal);
					specularResult += glm::pow(glm::max(glm::dot(specularR, observerVector), 0.0f), specular_brightness) * lightIntensity * specular;
				}
			}

			glm::vec3 resultColor = glm::clamp(ambientResult + diffuseResult + specularResult, 0.0f, 1.0f);
			DrawColor = LeandroColor(resultColor.x, resultColor.y, resultColor.z, 1.0f);
		}
		else {
			DrawColor = LeandroColor(meshTeste.GetColor().x, meshTeste.GetColor().y, meshTeste.GetColor().z, meshTeste.GetColor().w);
		}

		if (meshTeste.GetFill()) {
			FillPoly(currentVertices, minMaxFace);
		}
		if (meshTeste.GetOutline()) {
			DrawColor = LeandroColor(meshTeste.GetOutlineColor().x, meshTeste.GetOutlineColor().y, meshTeste.GetOutlineColor().z, meshTeste.GetOutlineColor().w);
			Draw3DLinesWithVertices(currentVertices);
		}
	}
}

void Renderer3D::FindIntersections(const std::vector<glm::vec3>& vertices, glm::vec2 yMinMax)
{
	int meshYMin = int(yMinMax.x);
	int meshYMax = int(yMinMax.y);

	for (int i = 0; i < vertices.size(); i++) {
		glm::vec3 source = vertices[i];
		glm::vec3 point = vertices[(i + 1) % vertices.size()];

		if (point.y == source.y) { continue; }

		// Aqui eu tenho os dois pontos
		if (point.y < source.y) { // point é mais alto que source; faz troca
			std::swap(source, point);
		}
		LE_CORE_ASSERT(point.y >= source.y, "Y FINAL da aresta maior que o INICIAL!");

		int ymin = (int)std::ceil(source.y);
		int ymax = (int)std::ceil(point.y);

		float dY = (point.y - source.y);

		float Tx = (point.x - source.x) / dY;
		float Tz = (point.z - source.z) / dY;
		//float Tr = (point.color.x - source.color.x) / dY;
		//float Tg = (point.color.y - source.color.y) / dY;
		//float Tb = (point.color.z - source.color.z) / dY;

		float new_x = source.x;
		float new_z = source.z;
		//float new_r = source.color.x;
		//float new_g = source.color.y;
		//float new_b = source.color.z;

		float preStep = (float)ymin - source.y;
		new_x += preStep * Tx;
		new_z += preStep * Tz;

		// Pegar as interseções
		for (int y = ymin; y < ymax; y++) { // Scanlines de 0 à Ns-1
			//SDL_SetRenderDrawColorFloat(renderer, 1, 0, 0, SDL_ALPHA_OPAQUE_FLOAT);
			//SDL_RenderLine(renderer, new_x, ymin + i, new_x, ymin+i);
			//SDL_RenderPoint(renderer, new_x, ymin + i);
			if (y > 0 && y < MainWindow->Height) {
				intersecoes[y].push_back({new_x, new_z});
				//LE_CORE_ASSERT(y > 0 && y < height, "Coordenada Y com valor INVALIDO!");
			}
			new_x += Tx;
			new_z += Tz;
			//new_r += Tr;
			//new_g += Tg;
			//new_b += Tb;
		}
	}
}

void Renderer3D::FillPoly(const std::vector<glm::vec3>& vertices, glm::vec2 minMaxY)
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
		std::sort(intersecoes[i].begin(), intersecoes[i].end());
	}

	// Fazer o fill no poly
	for (int y = yMin; y < yMax; y++) {
		if (y < 0 || y >= MainWindow->Height) { continue; }
		std::vector<std::pair<float, float>>& lista = intersecoes[y];

		for (int i = 0; i + 1 < lista.size(); i += 2) {
			//if (i + 1 < lista.size()) {
			//	SDL_RenderPoint(renderer, (int)lista[i].x, y);
			//	SDL_RenderPoint(renderer, (int)lista[i + 1].x, y);
			//}

			float dx = lista[i + 1].first - lista[i].first;
			float Tz = (lista[i + 1].second - lista[i].second) / dx;

			float currentZ = lista[i].second;

			int xStart = (int)std::ceil(lista[i].first);
			int xEnd = (int)std::ceil(lista[i + 1].first);

			currentZ += ((float)xStart - lista[i].first) * (dx > 0.0001f ? Tz : 1.0f);
			// utilizando o buffer
			for (int x = xStart; x < xEnd; x++) {
				if (x <= 0 || x >= MainWindow->Width) { continue; }
				if (y <= 0 || y >= MainWindow->Height) { continue; }
				if (currentZ < ZBuffer->at(y * MainWindow->Width + x)) {
					Buffer->at(y * MainWindow->Width + x) = DrawColor;
					ZBuffer->at(y * MainWindow->Width + x) = currentZ;
				}
				currentZ += (dx > 0.0001f ? Tz : 0.0f);
			}
			//SDL_RenderLine(renderer, lista[i], (float)y, lista[i + 1], (float)y);
		}
		intersecoes[y].clear();
	}
}