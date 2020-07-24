#pragma once
#include "shapes.h"

// Adds a shape/object to the ShapeLinker list.
// @param start Starting entry of the list. Make sure that this list, when it's empty, is NULL
// @param item Pointer to a shape/object. See shapes.h for all shapes
// @param type Type of the item field. See ShapeTypes in shapes.h
int ShapeLinkAdd(ShapeLinker_t **start, void *item, u8 type);

// returns an index of a list, kinda like array[offset]
ShapeLinker_t *ShapeLinkOffset(ShapeLinker_t *start, int offset);

// Cleans up a ShapeLinekr list. DataTypes are not cleaned up
void ShapeLinkDispose(ShapeLinker_t **start);

// Counts the amount of entries in a list
int ShapeLinkCount(ShapeLinker_t *start);

// Merges 2 ShapeLinker lists
void ShapeLinkMergeLists(ShapeLinker_t **dst, ShapeLinker_t *add);

// Finds a ShapeType in a ShapeLinker list
ShapeLinker_t *ShapeLinkFind(ShapeLinker_t  *start, int find);