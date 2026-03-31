/*
 * 控件池区域
 *
 * 能够容纳别的控件，不需要重写flush()方法
 */
#include <memory>
#include <unordered_map>
#include "lci/Area.h"
#include "lci/Drawable.h"
namespace qing {
	class PoolArea: public Area{
	public:
		PoolArea(Drawable *d, int w, int h, int x, int y, int rotate, int fontsize)
		:Area(d, w, h, x, y, rotate, fontsize) {};
		void add(const std::string& name, const std::shared_ptr<Area>& item) {
			umap.insert({name, item});
		}

		/* 访问控件 */
		std::shared_ptr<Area>& operator[](const std::string& key) {
			return umap.at(key);
		}

		/* 访问控件 - const */
		const std::shared_ptr<Area>& operator[](const std::string& key) const {
			return umap.at(key);
		}

		/* 安全访问 - 不抛异常 */
		//std::shard_ptr<Area> get(const std::string& key) const {}

		/* 检查是否存在 */
		bool contains(const std::string& key) const {
			return umap.find(key) != umap.end();
		}

		/* 获取控件数量 */
		size_t size() const { return umap.size(); }

		/* 迭代器支持 - 范围for循环 */
		std::unordered_map<std::string, std::shared_ptr<Area>>::iterator begin() { return umap.begin(); }
		std::unordered_map<std::string, std::shared_ptr<Area>>::iterator end()   { return umap.end();   }

		/* 迭代器支持 - const版本 */
		std::unordered_map<std::string, std::shared_ptr<Area>>::const_iterator begin() const { return umap.begin(); }
		std::unordered_map<std::string, std::shared_ptr<Area>>::const_iterator end()   const { return umap.end();   }

		/* FIXME: cbegin/cend 支持 */

	private:
		std::unordered_map<std::string, std::shared_ptr<Area>> umap;
	};
}
