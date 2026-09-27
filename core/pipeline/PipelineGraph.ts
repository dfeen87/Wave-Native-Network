import { NodeInterface } from "./NodeInterface";

export class PipelineGraph {
    private nodes: Map<string, NodeInterface> = new Map();
    private adjacency: Map<string, string[]> = new Map();
    private inDegree: Map<string, number> = new Map();

    public addNode(node: NodeInterface): void {
        if (this.nodes.has(node.id)) {
            throw new Error(`Node with id '${node.id}' already exists in graph.`);
        }
        this.nodes.set(node.id, node);
        if (!this.adjacency.has(node.id)) {
            this.adjacency.set(node.id, []);
        }
        if (!this.inDegree.has(node.id)) {
            this.inDegree.set(node.id, 0);
        }
    }

    public addEdge(fromId: string, toId: string): void {
        if (!this.nodes.has(fromId) || !this.nodes.has(toId)) {
            throw new Error(`Nodes ${fromId} and ${toId} must exist in graph before adding edge.`);
        }
        this.adjacency.get(fromId)!.push(toId);
        this.inDegree.set(toId, (this.inDegree.get(toId) || 0) + 1);
    }

    public getNode(id: string): NodeInterface | undefined {
        return this.nodes.get(id);
    }

    public getTopologicalOrder(): NodeInterface[] {
        const inDegCopy = new Map(this.inDegree);
        const queue: string[] = [];

        for (const [id, deg] of inDegCopy.entries()) {
            if (deg === 0) {
                queue.push(id);
            }
        }

        const result: NodeInterface[] = [];
        while (queue.length > 0) {
            const currId = queue.shift()!;
            result.push(this.nodes.get(currId)!);

            const neighbors = this.adjacency.get(currId) || [];
            for (const neighbor of neighbors) {
                const newDeg = (inDegCopy.get(neighbor) || 0) - 1;
                inDegCopy.set(neighbor, newDeg);
                if (newDeg === 0) {
                    queue.push(neighbor);
                }
            }
        }

        if (result.length !== this.nodes.size) {
            throw new Error("PipelineGraph contains a cycle.");
        }

        return result;
    }
}
