from pathlib import Path

path = Path("src/gameLayer/multyPlayer/tick.cpp")
text = path.read_text(encoding="utf-8")

old_decl = "\t\t\t\t\tglm::ivec2 newChunk = determineChunkThatIsEntityIn(e.second.getPosition());\n"
if text.count(old_decl) != 1:
    raise SystemExit(f"expected one newChunk declaration, found {text.count(old_decl)}")
text = text.replace(old_decl, "", 1)

old_block = r'''						if (initialChunk != newChunk)
						{
							//std::cout << "Prepare to move\n";
							auto chunk = chunkCache.getChunkOrGetNull(newChunk.x, newChunk.y);
							
							if (chunk)
							{
								//std::cout << "Found!\n";

								//move entity in another chunk
								auto member = memberSelector(chunk->entityData);
								member->insert({e.first, e.second});
								chunkCache.entityChunkPositions[e.first] = newChunk;

							}
							else
							{
								// Do not let a simulated entity fall out of the loaded world and then
								// disappear when the per-region orphan container is destroyed. Keep it
								// inside the last authoritative loaded chunk until streaming catches up.
								const glm::dvec3 clampedPosition = mie::dataIntegrity::clampEntityPositionToChunk(
									e.second.getPosition(), initialChunk);
								e.second.entity.position = clampedPosition;
								e.second.entity.lastPosition = clampedPosition;
								chunkCache.entityChunkPositions[e.first] = initialChunk;
								++it;
								continue;
							}


							it = container.erase(it);
						}
						else
						{
							++it;
						}
'''

new_block = r'''						using EntityValue = std::remove_reference_t<decltype(e.second.entity)>;
						if constexpr (hasPositionBasedID<EntityValue>)
						{
							// Position-based entities (for example TrainingDummy) are anchored to
							// their authoritative block/chunk and must never enter movement streaming.
							chunkCache.entityChunkPositions[e.first] = initialChunk;
							++it;
						}
						else
						{
							const glm::ivec2 newChunk = determineChunkThatIsEntityIn(e.second.getPosition());
							if (initialChunk != newChunk)
							{
								//std::cout << "Prepare to move\n";
								auto chunk = chunkCache.getChunkOrGetNull(newChunk.x, newChunk.y);
								
								if (chunk)
								{
									//std::cout << "Found!\n";

									//move entity in another chunk
									auto member = memberSelector(chunk->entityData);
									member->insert({e.first, e.second});
									chunkCache.entityChunkPositions[e.first] = newChunk;
								}
								else
								{
									// Do not let a simulated mobile entity fall out of the loaded world and
									// disappear when the per-region orphan container is destroyed. Keep it
									// inside the last authoritative loaded chunk until streaming catches up.
									const glm::dvec3 clampedPosition = mie::dataIntegrity::clampEntityPositionToChunk(
										e.second.getPosition(), initialChunk);
									e.second.entity.position = clampedPosition;
									e.second.entity.lastPosition = clampedPosition;
									chunkCache.entityChunkPositions[e.first] = initialChunk;
									++it;
									continue;
								}

								it = container.erase(it);
							}
							else
							{
								++it;
							}
						}
'''

if text.count(old_block) != 1:
    raise SystemExit(f"expected one movement block, found {text.count(old_block)}")
text = text.replace(old_block, new_block, 1)
path.write_text(text, encoding="utf-8")
