/**
 * Copyright (c) 2018-present, Apollo Ellis
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <iostream>
#include "thorax_bridge.h"
#include "renderer/build.h"

Grid::TextGrid* GetTextGridCPU(thorax_ai::Array<thorax_ai::Shape> &shapes, thorax_ai::Array<Grid::GlyphGrid> &glyphGrids, thorax_ai::Array<Grid::GlyphGridCell> &glyphGridCells,
	thorax_ai::Array<Grid::shape_ptr> &shapePtrs){
	
	Grid::Shape *cpuShapes = new Grid::Shape[shapes.size];
	for (int i = 0; i < shapes.size; i++) {
		cpuShapes[i].FromShape(shapes[i].p0v1p2v3.data.m256_f32);
	}
	Grid::TextGrid *textGrid = new Grid::TextGrid;
	textGrid->shapes = cpuShapes;
	textGrid->glyph_grids = glyphGrids.data;
	textGrid->glyph_grid_cells = glyphGridCells.data;
	textGrid->shape_ptrs = shapePtrs.data;
	textGrid->glyph_grid_count = (int)glyphGrids.size;
	textGrid->glyph_grid_cell_count = (int)glyphGridCells.size;
	textGrid->shape_count = (int)shapes.size;
	textGrid->shape_ptr_count = (int)shapePtrs.size;
	return textGrid;
}

//CALLER HAS TO HANDLE DATA DEALLOC USING DELETE[]
const char* readFile(const char* filePath, int* size)
{
	FILE* f = nullptr;
	int result = fopen_s(&f, filePath, "rb");
	if (result != 0) {
		std::cerr << "Failed to open file: " << filePath << ": " << result << "\n";
	}

	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	rewind(f);
	const char* data = new char[*size];
	fread_s((void*)data, *size, sizeof(char), *size, f);
	fclose(f);
	return data;
}

Grid::TextGrid* InitializeText(std::string path, std::string filename)
{
	std::string textfilename = "Times\ New\ Roman.ttf";
	const Font *font;
	{
		int dataSize = 0;
		const char* data = readFile((path + textfilename).c_str(), &dataSize);
		font = Font::AsFont(data);
	}
	thorax_ai::FontRenderInfo *ft_render_info = new thorax_ai::FontRenderInfo();
	ft_render_info->Initialize(font);

	int textLength = 0;
	const char* textData = nullptr;
	{ textData = readFile((path + filename).c_str(), &textLength);
	}

	//if (!textData) {
	//	return nullptr;
	//}

	unsigned points[4096];
	int hres = 0;
	int vresmax = 0;
	int hresmax = 0;
	int counts = 0;
	for (int i = 0; i < textLength; i++)
	{
		if (textData[i] < 0) {
			std::cerr << "Invalid character '" << textData[i] << "' at position " << i
			          << ". Are you sure the input is encoded in ASCII?\n";
			continue;
		}
		if (textData[i] == '\r') {
			continue;
		}

		if (textData[i] == '\n') {
			vresmax++;
			hres = 0;
		}
		else {
			hres++;
			hresmax = hresmax < hres ? hres : hresmax;
		}
		points[counts] = textData[i];
		counts++;
	}

	int refs = (int)ft_render_info->LayoutGlyphs((Grid::GridRef*)NULL, 0, (const unsigned *)points, counts);
	Grid::GridRef *glyphRefs = new Grid::GridRef[refs];
	int numRefs = ft_render_info->LayoutGlyphs(glyphRefs, 0, (const unsigned *)points, counts);

	Grid::TextGrid *tgrid = GetTextGridCPU(ft_render_info->shapes, ft_render_info->glyph_grids,
		ft_render_info->glyph_grid_cells, ft_render_info->shape_ptrs);
	GridBuild(tgrid, glyphRefs, numRefs, hresmax, vresmax);

	//delete[] textData;
	return tgrid;
}