local hash = {wei = {},shu = {},wu = {},qun = {}}
for _,general in pairs(sgs.Sanguosha:getLimitedGeneralNames())do
	if general:startsWith("lord_") then continue end
	if hash[sgs.Sanguosha:getGeneral(general):getKingdom()] then
		table.insert(hash[sgs.Sanguosha:getGeneral(general):getKingdom()],general)
	end
end
function getRandomGenerals (n,kingdom,exceptions)
	hash[kingdom] = table.Shuffle(hash[kingdom])
	local result = {}
	for _,general in pairs(hash[kingdom])do
		if #result == n then break end
		if not table.contains(exceptions,general) then
			table.insert(result,general)
		end
	end
	return result
end

XmodeRule = sgs.CreateTriggerSkill{
	name = "XmodeRule",
	events = {sgs.BuryVictim},
	on_effect = function(self, evnet, room, player, data,ask_who)
		player:bury()
		local times = room:getTag(player:getKingdom().."_Change"):toInt()
		player:speak(times)
		if times >= 3 then return false end
		local used = room:getTag("Xmode_UsedGeneral"):toString():split("+")
		local random_general = getRandomGenerals(sgs.GetConfig("HegemonyMaxChoice",0),player:getKingdom(),used)
		local choice = room:askForGeneral(player,table.concat(random_general,"+"),nil,false):split("+")
		table.insertTable(used,choice)
		room:setTag("Xmode_UsedGeneral",sgs.QVariant(table.concat(used,"+")))
		room:doDragonPhoenix(player,choice[1], choice[2],true,player:getKingdom(),false,"",true)
		player:drawCards(4)
		room:broadcastProperty(player,"kingdom")
		times = times + 1
		room:setTag(player:getKingdom().."_Change",sgs.QVariant(times))
		return true --不知道能不能处理飞龙
	end,
	priority = 1,
}
Xmode = {
	name = "Xmode_hegemony",
	expose_role = false,
	player_count = 8,
	random_seat = true,
	rule = XmodeRule,
	on_assign = function(self, room)
		local generals, generals2, kingdoms = {},{},{}
		local kingdom = {"wei","shu","wu","qun",}
		local rules_count = {["wei"] = 0,["shu"] = 0,["wu"] = 0,["qun"] = 0}
		for i = 1, 8, 1 do
			local role = kingdom[math.random(1,#kingdom)]
			rules_count[role] = rules_count[role] + 1
			if rules_count[role] == 2 then table.removeOne(kingdom,role) end
			table.insert(kingdoms, role)
		end
		--上面的代码是为了随机分配国家，每个势力有两个人。
		--相信大家都能看懂。
		local selected = {}
		for i = 1,8,1 do --开始给每名玩家分配待选择的武将。
			local player = room:getPlayers():at(i-1) --获取相关玩家
			player:clearSelected()  --清除已经分配的武将
			--如果不清除的话可能会获得上次askForGeneral的武将。
			local random_general = getRandomGenerals(sgs.GetConfig("HegemonyMaxChoice",0),kingdoms[i],selected)
			--随机获得HegemonyMaxChoice个武将。
			--函数getRandomGenerals在本文件内定义，可以参考之。
			for _,general in pairs(random_general)do
				player:addToSelected(general)
				--这个函数就是把武将分发给相关玩家。
				--分发的武将会在选将框中出现。
				table.insert(selected,general)
			end
		end
		room:chooseGenerals(room:getPlayers(),true,true)
		--这部分将在附录A中介绍。
		for i = 1,8,1 do --依次设置genera1、general2。
			local player = room:getPlayers():at(i-1)
			generals[i] = player:getGeneralName() --获取武将，这个是chooseGenerals分配的。
			generals2[i] = player:getGeneral2Name() --同上
			local used = room:getTag("Xmode_UsedGeneral"):toString():split("+") 
			table.insert(used,generals[i]) --设置返回值generals。
			table.insert(used,generals2[i])	--设置返回值generals2。
			room:setTag("Xmode_UsedGeneral",sgs.QVariant(table.concat(used,"+"))) --记录使用过的武将。
		end
		return generals, generals2,kingdoms
	end,
}
sgs.LoadTranslationTable{
	["Xmode_hegemony"] = "一统天下",
}
return sgs.CreateLuaScenario(Xmode)